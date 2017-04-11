#ifndef _SERVICECONTROL_H
#define _SERVICECONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_CTRL_INSTALL 0x01
#define DRIVER_CTRL_REMOVE  0X02

	int InstallDriver(SC_HANDLE schSCManager, LPCSTR serviceName, LPCSTR imagePath);

	int RemoveDriver(SC_HANDLE schSCManager, LPCSTR serviceName);

	int ManageDriver(LPCTSTR  driverName, LPCTSTR driverLocation, int ctrlCode);

	BOOLEAN
		SetupDriverName(
		_Inout_updates_bytes_all_(BufferLength) PTCHAR DriverLocation,
		_In_ ULONG BufferLength
		);

#ifdef __cplusplus
}
#endif

#endif /* _SERVICECONTROL_H */