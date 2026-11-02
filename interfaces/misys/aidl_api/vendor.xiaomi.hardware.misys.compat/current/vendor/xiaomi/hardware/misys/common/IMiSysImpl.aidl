///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.xiaomi.hardware.misys.common;
@VintfStability
interface IMiSysImpl {
  vendor.xiaomi.hardware.misys.common.FileInfo[] DirListFiles(String path);
  void EraseFileOrDirectory(String path, String fileName);
  String MiSysReadFile(String path, String fileName);
  void MiSysWriteFile(String path, String fileName, String content, int length, byte flag);
  boolean IsExists(String path, String fileName);
  void MiSysCreateFolder(String path, String folderName);
  byte[] MiSysReadBuffer(String path, String fileName);
  void MiSysWriteBuffer(String path, String fileName, in byte[] buffer, long length);
  boolean SetVirtualCameraConfig(int width, int height, double fps);
  boolean ConnectVirtualCamera();
  boolean DisconnectVirtualCamera();
  boolean OnFrameData(in byte[] frame, int length);
  void RegisterVCameraCallback(vendor.xiaomi.hardware.misys.common.IVCameraCallback callback);
  void UnregisterVCameraCallback();
  long GetFileSize(String path, String fileName);
  void MiSysSetProp(String key, String value);
  void ReadFromFile(in vendor.xiaomi.hardware.misys.common.Ashmem buffer, String path, String fileName);
  void WriteToFile(in vendor.xiaomi.hardware.misys.common.Ashmem buffer, String path, String fileName, long length);
  long GetPartitionSize(String path, String partition);
  void MiSysCreateFolderMode(String path, String folderName, int mode);
  void CopyFile(String src, String dst);
  vendor.xiaomi.hardware.misys.common.FileInfo[] DirIntgrFiles(String path);
}
