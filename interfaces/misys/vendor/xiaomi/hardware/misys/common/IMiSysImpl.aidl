/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.xiaomi.hardware.misys.common;

import vendor.xiaomi.hardware.misys.common.Ashmem;
import vendor.xiaomi.hardware.misys.common.FileInfo;
import vendor.xiaomi.hardware.misys.common.IVCameraCallback;

// Declaration order = transaction codes 1..22 of the stock interface (MiuiCamera Java stub).
@VintfStability
interface IMiSysImpl {
    FileInfo[] DirListFiles(String path);
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
    void RegisterVCameraCallback(IVCameraCallback callback);
    void UnregisterVCameraCallback();
    long GetFileSize(String path, String fileName);
    void MiSysSetProp(String key, String value);
    void ReadFromFile(in Ashmem buffer, String path, String fileName);
    void WriteToFile(in Ashmem buffer, String path, String fileName, long length);
    long GetPartitionSize(String path, String partition);
    void MiSysCreateFolderMode(String path, String folderName, int mode);
    void CopyFile(String src, String dst);
    FileInfo[] DirIntgrFiles(String path);
}
