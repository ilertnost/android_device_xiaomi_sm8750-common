/*
 * libmfpshim: makes the stock Xiaomi odm fingerprint daemon (mfp-daemon, closed, IFingerprint V5) usable under
 * the AOSP 17 framework on dada (Xiaomi 15, Qualcomm 3D Sonic under-display sensor).
 *
 * Problem: AOSP 17 sends UDFPS touches with ISession::onPointerDownWithContext / onPointerUpWithContext (HAL
 * interface >= 2). mfp-daemon only acts on the old onPointerDown / onPointerUp (the path MIUI/HyperOS uses), its
 * *WithContext methods do nothing -> enrollment stops after the first touch.
 *
 * mfp-daemon also reports halHandlesDisplayTouches=true, so AOSP drops all pointer events: the shim reports false.
 *
 * Even with pointer events the sensor only captures while the daemon has FOD touch enabled in the touch driver.
 * MIUI drives that through the Xiaomi extension service (vendor.xiaomi.hardware.fingerprintextension.
 * IXiaomiFingerprint/default, registered by mfp-daemon itself): extCmd(cmd, value) sets the daemon's FodEngine
 * condition <cmd>. Probed on the phone: cmd 1 = gxzwAnim (fingerprint icon shown), 2 = colorfade, 4 = FP (operation
 * running), 5 = lockout; the daemon enables FOD touch ("checkFodTouchStatus ... TouchEnable:1", "HandleFodTouchCtl:1")
 * only with FP=1, gxzwAnim=1, colorfade=0, lockout=0 and the panel on. MIUI sets FP and gxzwAnim while its
 * fingerprint UI is up; the shim does the same: on at the start of enroll / authenticate / detectInteraction, off on
 * cancel and on the terminal callbacks (error, enrollment done, success, lockout, interaction detected, session
 * closed). For that it also wraps the session callback and the returned cancellation signal.
 *
 * Fix: loaded deterministically as a DT_NEEDED dependency of mfp-daemon. It interposes AServiceManager_addService /
 * AServiceManager_registerLazyService; when the daemon registers "android.hardware.biometrics.fingerprint.
 * IFingerprint/default", the local IFingerprint object is wrapped: every call is forwarded unchanged, except
 * onPointer{Down,Up,Cancel}WithContext -> onPointerDown / onPointerUp, and {enroll,authenticate,detectInteraction}
 * WithContext fall back to the plain calls if the daemon does not implement them. The daemon code is not changed.
 */
#define LOG_TAG "mfpshim"

#include <dlfcn.h>
#include <string.h>

#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnSession.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>
#include <android/binder_manager.h>
#include <log/log.h>

using ::aidl::android::hardware::biometrics::common::BnCancellationSignal;
using ::aidl::android::hardware::biometrics::common::ICancellationSignal;
using ::aidl::android::hardware::biometrics::common::OperationContext;
using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;
using ::aidl::android::hardware::biometrics::fingerprint::AuthenticateSuccess;
using ::aidl::android::hardware::biometrics::fingerprint::BnFingerprint;
using ::aidl::android::hardware::biometrics::fingerprint::BnSession;
using ::aidl::android::hardware::biometrics::fingerprint::BnSessionCallback;
using ::aidl::android::hardware::biometrics::fingerprint::Error;
using ::aidl::android::hardware::biometrics::fingerprint::IFingerprint;
using ::aidl::android::hardware::biometrics::fingerprint::ISession;
using ::aidl::android::hardware::biometrics::fingerprint::ISessionCallback;
using ::aidl::android::hardware::biometrics::fingerprint::PointerContext;
using ::aidl::android::hardware::biometrics::fingerprint::SensorProps;
using ::aidl::android::hardware::keymaster::HardwareAuthToken;

namespace {

constexpr char kXiaomiExtInstance[] = "vendor.xiaomi.hardware.fingerprintextension.IXiaomiFingerprint/default";
constexpr int32_t kCondGxzwAnim = 1;  // fingerprint icon shown
constexpr int32_t kCondFp = 4;        // fingerprint operation running
ndk::SpAIBinder gXiaomiExt;           // the daemon's own local IXiaomiFingerprint object

// IXiaomiFingerprint has a single method: int extCmd(int cmd, int param) -> transaction FIRST_CALL_TRANSACTION.
// The object is local (same process) and already bound to its class, so a raw transaction is enough.
int32_t xiaomiExtCmd(int32_t cmd, int32_t param) {
    if (gXiaomiExt.get() == nullptr) return -1;
    AParcel* in = nullptr;
    if (AIBinder_prepareTransaction(gXiaomiExt.get(), &in) != STATUS_OK) return -2;
    AParcel_writeInt32(in, cmd);
    AParcel_writeInt32(in, param);
    AParcel* out = nullptr;
    binder_status_t st = AIBinder_transact(gXiaomiExt.get(), FIRST_CALL_TRANSACTION, &in, &out, 0);
    int32_t ret = -3;
    if (st == STATUS_OK && out != nullptr) {
        AStatus* status = nullptr;
        if (AParcel_readStatusHeader(out, &status) == STATUS_OK && AStatus_isOk(status)) {
            AParcel_readInt32(out, &ret);
        }
        if (status != nullptr) AStatus_delete(status);
    }
    if (out != nullptr) AParcel_delete(out);
    return ret;
}

// FOD touch on while a fingerprint operation runs (what MIUI does while its fingerprint UI is shown).
void fodUi(bool on, const char* why) {
    int32_t fp = xiaomiExtCmd(kCondFp, on ? 1 : 0);
    int32_t anim = xiaomiExtCmd(kCondGxzwAnim, on ? 1 : 0);
    ALOGD("FOD touch %s (%s): extCmd FP=%d gxzwAnim=%d", on ? "on" : "off", why, fp, anim);
}

bool notImplemented(const ndk::ScopedAStatus& s) {
    return s.getStatus() == STATUS_UNKNOWN_TRANSACTION ||
           s.getExceptionCode() == EX_UNSUPPORTED_OPERATION;
}

class CancelShim : public BnCancellationSignal {
  public:
    explicit CancelShim(std::shared_ptr<ICancellationSignal> real) : r(std::move(real)) {}
    ndk::ScopedAStatus cancel() override {
        fodUi(false, "cancel");
        return r->cancel();
    }

  private:
    std::shared_ptr<ICancellationSignal> r;
};

void wrapCancel(std::shared_ptr<ICancellationSignal>* out) {
    if (out != nullptr && *out) *out = ndk::SharedRefBase::make<CancelShim>(*out);
}

// FOD touch goes off before the framework hears about the end of an operation, so that the off can never land after
// the on of the operation the framework starts next.
class CallbackShim : public BnSessionCallback {
  public:
    explicit CallbackShim(std::shared_ptr<ISessionCallback> cb) : c(std::move(cb)) {}

    ndk::ScopedAStatus onChallengeGenerated(int64_t ch) override { return c->onChallengeGenerated(ch); }
    ndk::ScopedAStatus onChallengeRevoked(int64_t ch) override { return c->onChallengeRevoked(ch); }
    ndk::ScopedAStatus onAcquired(AcquiredInfo info, int32_t vendorCode) override {
        return c->onAcquired(info, vendorCode);
    }
    ndk::ScopedAStatus onError(Error error, int32_t vendorCode) override {
        fodUi(false, "onError");
        return c->onError(error, vendorCode);
    }
    ndk::ScopedAStatus onEnrollmentProgress(int32_t id, int32_t remaining) override {
        ALOGD("onEnrollmentProgress id=%d remaining=%d", id, remaining);
        if (remaining == 0) fodUi(false, "enrolled");
        return c->onEnrollmentProgress(id, remaining);
    }
    ndk::ScopedAStatus onAuthenticationSucceeded(int32_t id, const HardwareAuthToken& hat) override {
        fodUi(false, "authenticated");
        return c->onAuthenticationSucceeded(id, hat);
    }
    ndk::ScopedAStatus onAuthenticationFailed() override { return c->onAuthenticationFailed(); }
    ndk::ScopedAStatus onLockoutTimed(int64_t ms) override {
        fodUi(false, "lockout");
        return c->onLockoutTimed(ms);
    }
    ndk::ScopedAStatus onLockoutPermanent() override {
        fodUi(false, "lockout");
        return c->onLockoutPermanent();
    }
    ndk::ScopedAStatus onLockoutCleared() override { return c->onLockoutCleared(); }
    ndk::ScopedAStatus onInteractionDetected() override {
        fodUi(false, "interaction");
        return c->onInteractionDetected();
    }
    ndk::ScopedAStatus onEnrollmentsEnumerated(const std::vector<int32_t>& ids) override {
        return c->onEnrollmentsEnumerated(ids);
    }
    ndk::ScopedAStatus onEnrollmentsRemoved(const std::vector<int32_t>& ids) override {
        return c->onEnrollmentsRemoved(ids);
    }
    ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t id) override { return c->onAuthenticatorIdRetrieved(id); }
    ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t id) override {
        return c->onAuthenticatorIdInvalidated(id);
    }
    ndk::ScopedAStatus onSessionClosed() override {
        fodUi(false, "session closed");
        return c->onSessionClosed();
    }
    ndk::ScopedAStatus onAuthenticationSucceededWithResult(const AuthenticateSuccess& result) override {
        fodUi(false, "authenticated");
        return c->onAuthenticationSucceededWithResult(result);
    }

  private:
    std::shared_ptr<ISessionCallback> c;
};

class SessionShim : public BnSession {
  public:
    explicit SessionShim(std::shared_ptr<ISession> real) : r(std::move(real)) {}

    ndk::ScopedAStatus generateChallenge() override { return r->generateChallenge(); }
    ndk::ScopedAStatus revokeChallenge(int64_t c) override { return r->revokeChallenge(c); }
    ndk::ScopedAStatus enroll(const HardwareAuthToken& hat,
                              std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "enroll");
        auto s = r->enroll(hat, out);
        wrapCancel(out);
        return s;
    }
    ndk::ScopedAStatus authenticate(int64_t op, std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "authenticate");
        auto s = r->authenticate(op, out);
        wrapCancel(out);
        return s;
    }
    ndk::ScopedAStatus detectInteraction(std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "detectInteraction");
        auto s = r->detectInteraction(out);
        wrapCancel(out);
        return s;
    }
    ndk::ScopedAStatus enumerateEnrollments() override { return r->enumerateEnrollments(); }
    ndk::ScopedAStatus removeEnrollments(const std::vector<int32_t>& ids) override {
        return r->removeEnrollments(ids);
    }
    ndk::ScopedAStatus getAuthenticatorId() override { return r->getAuthenticatorId(); }
    ndk::ScopedAStatus invalidateAuthenticatorId() override { return r->invalidateAuthenticatorId(); }
    ndk::ScopedAStatus resetLockout(const HardwareAuthToken& hat) override { return r->resetLockout(hat); }
    ndk::ScopedAStatus close() override { return r->close(); }
    ndk::ScopedAStatus onPointerDown(int32_t id, int32_t x, int32_t y, float minor, float major) override {
        return r->onPointerDown(id, x, y, minor, major);
    }
    ndk::ScopedAStatus onPointerUp(int32_t id) override { return r->onPointerUp(id); }
    ndk::ScopedAStatus onUiReady() override { return r->onUiReady(); }

    ndk::ScopedAStatus authenticateWithContext(int64_t op, const OperationContext& ctx,
                                               std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "authenticate");
        auto s = r->authenticateWithContext(op, ctx, out);
        if (notImplemented(s)) s = r->authenticate(op, out);
        wrapCancel(out);
        return s;
    }
    ndk::ScopedAStatus enrollWithContext(const HardwareAuthToken& hat, const OperationContext& ctx,
                                         std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "enroll");
        auto s = r->enrollWithContext(hat, ctx, out);
        if (notImplemented(s)) s = r->enroll(hat, out);
        wrapCancel(out);
        return s;
    }
    ndk::ScopedAStatus detectInteractionWithContext(const OperationContext& ctx,
                                                    std::shared_ptr<ICancellationSignal>* out) override {
        fodUi(true, "detectInteraction");
        auto s = r->detectInteractionWithContext(ctx, out);
        if (notImplemented(s)) s = r->detectInteraction(out);
        wrapCancel(out);
        return s;
    }
    // The daemon only reacts to the old pointer calls.
    ndk::ScopedAStatus onPointerDownWithContext(const PointerContext& c) override {
        ALOGD("onPointerDownWithContext id=%d x=%.0f y=%.0f -> onPointerDown", c.pointerId, c.x, c.y);
        return r->onPointerDown(c.pointerId, static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), c.minor,
                                c.major);
    }
    ndk::ScopedAStatus onPointerUpWithContext(const PointerContext& c) override {
        ALOGD("onPointerUpWithContext id=%d -> onPointerUp", c.pointerId);
        return r->onPointerUp(c.pointerId);
    }
    ndk::ScopedAStatus onPointerCancelWithContext(const PointerContext& c) override {
        ALOGD("onPointerCancelWithContext id=%d -> onPointerUp", c.pointerId);
        return r->onPointerUp(c.pointerId);
    }
    ndk::ScopedAStatus onContextChanged(const OperationContext& ctx) override {
        r->onContextChanged(ctx);
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus setIgnoreDisplayTouches(bool ignore) override {
        r->setIgnoreDisplayTouches(ignore);
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::shared_ptr<ISession> r;
};

class FingerprintShim : public BnFingerprint {
  public:
    explicit FingerprintShim(std::shared_ptr<IFingerprint> real) : r(std::move(real)) {}

    ndk::ScopedAStatus getSensorProps(std::vector<SensorProps>* out) override {
        auto s = r->getSensorProps(out);
        if (s.isOk() && out) {
            for (auto& p : *out) {
                // mfp-daemon reports halHandlesDisplayTouches=true (under MIUI the touch press reaches it through the
                // touch driver); the AOSP framework then drops every pointer event (FingerprintProvider.onPointerDown).
                // Report false so that SystemUI's touches arrive here and are passed on as onPointerDown/Up.
                ALOGI("sensor %d: type=%d halHandlesDisplayTouches=%d halControlsIllumination=%d -> touches via framework",
                      p.commonProps.sensorId, static_cast<int>(p.sensorType), p.halHandlesDisplayTouches,
                      p.halControlsIllumination);
                p.halHandlesDisplayTouches = false;
            }
        }
        return s;
    }
    ndk::ScopedAStatus createSession(int32_t sensorId, int32_t userId,
                                     const std::shared_ptr<ISessionCallback>& cb,
                                     std::shared_ptr<ISession>* out) override {
        std::shared_ptr<ISession> real;
        auto s = r->createSession(sensorId, userId, ndk::SharedRefBase::make<CallbackShim>(cb), &real);
        if (!s.isOk()) {
            // e.g. the daemon links to the callback's death, which a local binder does not support
            ALOGE("createSession with the callback shim failed (%s), using the framework callback",
                  s.getDescription().c_str());
            s = r->createSession(sensorId, userId, cb, &real);
        }
        if (s.isOk() && real) {
            *out = ndk::SharedRefBase::make<SessionShim>(real);
            ALOGI("createSession(sensor %d, user %d): wrapped", sensorId, userId);
        }
        return s;
    }

  private:
    std::shared_ptr<IFingerprint> r;
};

constexpr char kInstance[] = "android.hardware.biometrics.fingerprint.IFingerprint/default";
std::shared_ptr<FingerprintShim> gShim;  // keeps the wrapper alive for the life of the daemon
ndk::SpAIBinder gShimBinder;             // keeps its AIBinder alive: asBinder() returns a temporary strong ref

AIBinder* maybeWrap(AIBinder* binder, const char* instance) {
    if (binder != nullptr && instance != nullptr && strcmp(instance, kXiaomiExtInstance) == 0) {
        AIBinder_incStrong(binder);  // SpAIBinder adopts one strong ref
        gXiaomiExt = ndk::SpAIBinder(binder);
        ALOGI("%s: captured (FOD touch conditions are set through it)", instance);
        return binder;  // registered unchanged
    }
    if (binder == nullptr || instance == nullptr || strcmp(instance, kInstance) != 0) return binder;
    AIBinder_incStrong(binder);  // SpAIBinder adopts one strong ref
    std::shared_ptr<IFingerprint> real = IFingerprint::fromBinder(ndk::SpAIBinder(binder));
    if (!real || real->isRemote()) {
        ALOGE("%s: not a local IFingerprint, registering it unchanged", instance);
        return binder;
    }
    gShim = ndk::SharedRefBase::make<FingerprintShim>(real);
    gShimBinder = gShim->asBinder();
    ALOGI("%s: registering the shim (old pointer calls for *WithContext)", instance);
    return gShimBinder.get();
}

}  // namespace

extern "C" binder_exception_t AServiceManager_addService(AIBinder* binder, const char* instance) {
    using Fn = binder_exception_t (*)(AIBinder*, const char*);
    static Fn real = reinterpret_cast<Fn>(dlvsym(RTLD_NEXT, "AServiceManager_addService", "LIBBINDER_NDK"));
    return real(maybeWrap(binder, instance), instance);
}

extern "C" binder_status_t AServiceManager_registerLazyService(AIBinder* binder, const char* instance) {
    using Fn = binder_status_t (*)(AIBinder*, const char*);
    static Fn real =
            reinterpret_cast<Fn>(dlvsym(RTLD_NEXT, "AServiceManager_registerLazyService", "LIBBINDER_NDK31"));
    return real(maybeWrap(binder, instance), instance);
}
