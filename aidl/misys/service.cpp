/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

// vendor.xiaomi.hardware.misys.common.IMiSysImpl/default for the stock camera app (MiuiCamera). Stock hosts it
// in hypsys_vendor (stability/charge-log daemon); this implements only the file calls the app makes, each
// confined to the app's files below /data/vendor/camera (paths.h). Everything else is unsupported.

#include <aidl/vendor/xiaomi/hardware/misys/common/BnMiSysImpl.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <cutils/ashmem.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <memory>

#include "paths.h"

namespace {

using ::aidl::vendor::xiaomi::hardware::misys::common::Ashmem;
using ::aidl::vendor::xiaomi::hardware::misys::common::BnMiSysImpl;
using ::aidl::vendor::xiaomi::hardware::misys::common::FileInfo;
using ::aidl::vendor::xiaomi::hardware::misys::common::IVCameraCallback;
using ::android::base::unique_fd;
using ::ndk::ScopedAStatus;

constexpr int64_t kMaxBuffer = 512 * 1024;  // byte[] replies have to fit the binder buffer
constexpr int64_t kMaxShared = 64 << 20;    // SharedMemory copies (fonts, watermark assets)

ScopedAStatus errnoStatus() {
    return ScopedAStatus::fromServiceSpecificError(errno);
}

ScopedAStatus illegalArgument() {
    return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ScopedAStatus unsupported() {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

// Checks the app's (dir, name) pair against the allowlist and opens the directory holding the target,
// walking down from /data/vendor without following any symlink. /data/vendor/camera itself only if allowRoot.
ScopedAStatus target(const std::string& dir, const std::string& name, bool allowRoot, unique_fd* parent,
                     std::string* leaf) {
    std::vector<std::string> comps;
    if (!misys::resolve(dir, name, &comps) || (comps.size() == 1 && !allowRoot)) {
        LOG(WARNING) << "rejected path " << dir << " + " << name;
        return ScopedAStatus::fromExceptionCode(EX_SECURITY);
    }
    constexpr int kDirFlags = O_PATH | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC;
    parent->reset(open(misys::kBase, kDirFlags));
    for (size_t i = 0; parent->ok() && i + 1 < comps.size(); i++) {
        parent->reset(openat(parent->get(), comps[i].c_str(), kDirFlags));
    }
    *leaf = comps.back();
    return parent->ok() ? ScopedAStatus::ok() : errnoStatus();
}

// Opens a regular file below parent: never through a symlink, never blocking on a FIFO.
unique_fd openFile(int parent, const std::string& leaf, int flags, struct stat* st) {
    unique_fd fd(openat(parent, leaf.c_str(), flags | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC, 0660));
    if (fd.ok() && (fstat(fd.get(), st) != 0 || !S_ISREG(st->st_mode))) {
        fd.reset();
        errno = EINVAL;
    }
    return fd;
}

// Deletes name below parent, directories recursively. Symlinks are never followed.
bool removeAt(int parent, const char* name, size_t depth) {
    struct stat st;
    if (fstatat(parent, name, &st, AT_SYMLINK_NOFOLLOW) != 0) return errno == ENOENT;
    if (!S_ISDIR(st.st_mode)) return unlinkat(parent, name, 0) == 0;
    if (depth > misys::kMaxDepth) {
        errno = ELOOP;
        return false;
    }
    const int fd = openat(parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    std::unique_ptr<DIR, decltype(&closedir)> dir(fd < 0 ? nullptr : fdopendir(fd), closedir);
    if (!dir) {
        if (fd >= 0) close(fd);
        return false;
    }
    bool ok = true;
    while (const dirent* e = readdir(dir.get())) {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0) {
            ok = removeAt(dirfd(dir.get()), e->d_name, depth + 1) && ok;
        }
    }
    dir.reset();
    return ok && unlinkat(parent, name, AT_REMOVEDIR) == 0;
}

// Maps the caller's SharedMemory after checking the claimed size against the real region size.
// shortcut: a client that shrinks its memfd during the copy makes this process SIGBUS (init restarts it);
// check F_SEAL_SHRINK first if anything other than the camera app ever gets access.
void* mapShared(const Ashmem& mem, int prot) {
    const int fd = mem.fd.get();
    if (fd < 0 || mem.size <= 0 || mem.size > kMaxShared || ashmem_get_size_region(fd) < mem.size) {
        errno = EINVAL;
        return MAP_FAILED;
    }
    return mmap(nullptr, mem.size, prot, MAP_SHARED, fd, 0);
}

class MiSysImpl : public BnMiSysImpl {
  public:
    ScopedAStatus EraseFileOrDirectory(const std::string& dir, const std::string& name) override {
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        return removeAt(parent.get(), leaf.c_str(), 0) ? ScopedAStatus::ok() : errnoStatus();
    }

    ScopedAStatus IsExists(const std::string& dir, const std::string& name, bool* ret) override {
        unique_fd parent;
        std::string leaf;
        auto s = target(dir, name, true, &parent, &leaf);
        if (s.getExceptionCode() == EX_SECURITY) return s;
        struct stat st;
        *ret = s.isOk() && fstatat(parent.get(), leaf.c_str(), &st, AT_SYMLINK_NOFOLLOW) == 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus MiSysCreateFolder(const std::string& dir, const std::string& name) override {
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        if (mkdirat(parent.get(), leaf.c_str(), 0770) != 0 && errno != EEXIST) return errnoStatus();
        return ScopedAStatus::ok();
    }

    ScopedAStatus GetFileSize(const std::string& dir, const std::string& name, int64_t* ret) override {
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        struct stat st;
        if (fstatat(parent.get(), leaf.c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0) return errnoStatus();
        if (!S_ISREG(st.st_mode)) return illegalArgument();
        *ret = st.st_size;
        return ScopedAStatus::ok();
    }

    ScopedAStatus MiSysReadBuffer(const std::string& dir, const std::string& name,
                                  std::vector<uint8_t>* ret) override {
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        struct stat st;
        unique_fd fd = openFile(parent.get(), leaf, O_RDONLY, &st);
        if (!fd.ok()) return errnoStatus();
        if (st.st_size > kMaxBuffer) return illegalArgument();
        ret->resize(st.st_size);
        return android::base::ReadFully(fd.get(), ret->data(), ret->size()) ? ScopedAStatus::ok()
                                                                            : errnoStatus();
    }

    ScopedAStatus MiSysWriteBuffer(const std::string& dir, const std::string& name,
                                   const std::vector<uint8_t>& buffer, int64_t length) override {
        if (length < 0 || static_cast<uint64_t>(length) > buffer.size()) return illegalArgument();
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        struct stat st;
        unique_fd fd = openFile(parent.get(), leaf, O_WRONLY | O_CREAT | O_TRUNC, &st);
        if (!fd.ok() || !android::base::WriteFully(fd.get(), buffer.data(), length)) return errnoStatus();
        return ScopedAStatus::ok();
    }

    ScopedAStatus ReadFromFile(const Ashmem& mem, const std::string& dir, const std::string& name) override {
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        struct stat st;
        unique_fd fd = openFile(parent.get(), leaf, O_RDONLY, &st);
        if (!fd.ok()) return errnoStatus();
        void* map = mapShared(mem, PROT_READ | PROT_WRITE);
        if (map == MAP_FAILED) return errnoStatus();
        const bool ok = android::base::ReadFully(fd.get(), map, std::min<int64_t>(mem.size, st.st_size));
        munmap(map, mem.size);
        return ok ? ScopedAStatus::ok() : errnoStatus();
    }

    ScopedAStatus WriteToFile(const Ashmem& mem, const std::string& dir, const std::string& name,
                              int64_t length) override {
        if (length < 0 || length > mem.size) return illegalArgument();
        unique_fd parent;
        std::string leaf;
        if (auto s = target(dir, name, false, &parent, &leaf); !s.isOk()) return s;
        void* map = mapShared(mem, PROT_READ);
        if (map == MAP_FAILED) return errnoStatus();
        struct stat st;
        unique_fd fd = openFile(parent.get(), leaf, O_WRONLY | O_CREAT | O_TRUNC, &st);
        const bool ok = fd.ok() && android::base::WriteFully(fd.get(), map, length);
        munmap(map, mem.size);
        return ok ? ScopedAStatus::ok() : errnoStatus();
    }

    // Not used by the camera app on this device (DirListFiles only for the /mnt/vendor/persist/camera
    // fallback, which holds calibration data and is deliberately not reachable) or not file related.
    ScopedAStatus DirListFiles(const std::string&, std::vector<FileInfo>*) override { return unsupported(); }
    ScopedAStatus MiSysReadFile(const std::string&, const std::string&, std::string*) override {
        return unsupported();
    }
    ScopedAStatus MiSysWriteFile(const std::string&, const std::string&, const std::string&, int32_t,
                                 int8_t) override {
        return unsupported();
    }
    ScopedAStatus SetVirtualCameraConfig(int32_t, int32_t, double, bool*) override { return unsupported(); }
    ScopedAStatus ConnectVirtualCamera(bool*) override { return unsupported(); }
    ScopedAStatus DisconnectVirtualCamera(bool*) override { return unsupported(); }
    ScopedAStatus OnFrameData(const std::vector<uint8_t>&, int32_t, bool*) override { return unsupported(); }
    ScopedAStatus RegisterVCameraCallback(const std::shared_ptr<IVCameraCallback>&) override {
        return unsupported();
    }
    ScopedAStatus UnregisterVCameraCallback() override { return unsupported(); }
    ScopedAStatus MiSysSetProp(const std::string&, const std::string&) override { return unsupported(); }
    ScopedAStatus GetPartitionSize(const std::string&, const std::string&, int64_t*) override {
        return unsupported();
    }
    ScopedAStatus MiSysCreateFolderMode(const std::string&, const std::string&, int32_t) override {
        return unsupported();
    }
    ScopedAStatus CopyFile(const std::string&, const std::string&) override { return unsupported(); }
    ScopedAStatus DirIntgrFiles(const std::string&, std::vector<FileInfo>*) override { return unsupported(); }
};

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);  // only the main thread: calls are serialized
    auto service = ndk::SharedRefBase::make<MiSysImpl>();
    const std::string instance = std::string(MiSysImpl::descriptor) + "/default";
    CHECK_EQ(AServiceManager_addService(service->asBinder().get(), instance.c_str()), STATUS_OK);
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // not reached
}
