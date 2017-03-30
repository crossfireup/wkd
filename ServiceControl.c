#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "comm.h"
#include "ServiceControl.h"

int InstallDriver(SC_HANDLE schSCManager,
	LPCSTR serviceName,
	LPCSTR imagePath)
{
	SC_HANDLE hService = NULL;
	DWORD dwErr = 0;

	hService = CreateService(schSCManager,
		serviceName,
		serviceName,
		SC_MANAGER_CREATE_SERVICE,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		imagePath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
		);

	if (NULL == hService){
		dwErr = GetLastError();

		if (ERROR_SERVICE_EXISTS == dwErr){
			MessageBoxPrintf(NULL, MB_OK, "Information", "Driver has installed already");
			return TRUE;
		}
		else if (ERROR_SERVICE_MARKED_FOR_DELETE == dwErr){
			MessageBoxPrintf(NULL, MB_OK, "Information", "Driver marked for delete");
			return FALSE;
		}
		else {
			DisplayError(_T("CreateService"));
			return FALSE;
		}
	}

	CloseServiceHandle(hService);

	return TRUE;
}

int
StartDriver(
_In_ SC_HANDLE    SchSCManager,
_In_ LPCTSTR      DriverName
)
{
	SC_HANDLE  schService;
	int retCode = 0;
	DWORD dwErr = 0;

	schService = OpenService(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
		);

	if (schService == NULL) {

		DisplayError(_T("OpenService"));

		return FALSE;
	}

	retCode = StartService(schService,     // service identifier
		0,              // number of arguments
		NULL            // pointer to arguments
		);

	if (!retCode) {

		dwErr = GetLastError();

		if (dwErr == ERROR_SERVICE_ALREADY_RUNNING) {
			retCode = TRUE;
		}
		else {
			DisplayError(_T("StartService"));
		}
	}

	CloseServiceHandle(schService);

	return retCode;
}

int StopDriver(
_In_ SC_HANDLE    SchSCManager,
_In_ LPCTSTR      DriverName
)
{
	SC_HANDLE       schService = NULL;
	int				retCode = 0;
	SERVICE_STATUS  serviceStatus = { 0 };

	schService = OpenService(SchSCManager,
		DriverName,
		SERVICE_STOP
		);

	if (schService == NULL) {

		DisplayError(_T("OpenService"));
		return FALSE;
	}

	retCode = ControlService(schService,
		SERVICE_CONTROL_STOP,
		&serviceStatus
		);
	if (!retCode) {
		DisplayError(_T("ControlServic"));
	}

	CloseServiceHandle(schService);

	return retCode;
} //  StopDriver


int RemoveDriver(SC_HANDLE schSCManager, LPCSTR serviceName)
{
	SC_HANDLE hService = NULL;
	DWORD dwErr = 0;
	int retCode = 0;

	hService = OpenService(schSCManager,
		serviceName,
		SERVICE_STOP | DELETE);

	if (NULL == hService){
		DisplayError(_T("OpenService"));
		return FALSE;
	}

	retCode = DeleteService(hService);

	if (retCode == FALSE){
		dwErr = GetLastError();
		if (dwErr == ERROR_SERVICE_MARKED_FOR_DELETE){
			MessageBoxPrintf(NULL, MB_OK, "Error", "The specified service has already been marked for deletion");
		}
		else {
			DisplayError(_T("DeleteService"));
		}
	}

	CloseServiceHandle(hService);

	return retCode;
}


int ManageDriver(LPCTSTR  driverName, LPCTSTR driverLocation, int ctrlCode)
{
	SC_HANDLE schSCManager = NULL;
	int retCode = TRUE;
	DWORD dwErr = 0;

	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_CREATE_SERVICE
		);

	if (schSCManager == NULL){
		DisplayError(_T("OpenSCManager"));
		return FALSE;
	}

	switch (ctrlCode){
	case DRIVER_CTRL_INSTALL:
		if (InstallDriver(schSCManager, driverName, driverLocation)){
			StartDriver(schSCManager, driverName);
		}
		break;

	case DRIVER_CTRL_REMOVE:
		StopDriver(schSCManager, driverName);
		RemoveDriver(schSCManager, driverName);
		break;

	default:
		MessageBoxPrintf(NULL, MB_OK, "Error", "ManageDriver error, not implemented: %x", ctrlCode);
		retCode = FALSE;
		break;
	}

	CloseServiceHandle(schSCManager);

	return(retCode);
}

BOOLEAN
SetupDriverName(
_Inout_updates_bytes_all_(BufferLength) PTCHAR DriverLocation,
_In_ ULONG BufferLength
)
{
	HANDLE hDevice = NULL;
	DWORD driverLocLen = 0;

	//
	// Get the current directory.
	//

	driverLocLen = GetCurrentDirectory(BufferLength,
		DriverLocation
		);

	if (driverLocLen == 0) {

		DisplayError(_T("GetCurrentDirectory"));

		return FALSE;
	}

	//
	// Setup path name to driver file.
	//
	if (FAILED(StringCchCat(DriverLocation, BufferLength, _T("\\"DRIVER_NAME".sys")))) {
		DisplayError(_T("StringCbCat"));
		return FALSE;
	}

	//
	// Insure driver file is in the specified directory.
	//

	if ((hDevice = CreateFile(DriverLocation,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		)) == INVALID_HANDLE_VALUE) {
		MessageBoxPrintf(NULL, MB_OK, "Error", "%s.sys is not loaded.\n", DRIVER_NAME);
		return FALSE;
	}

	CloseHandle(hDevice);

	return TRUE;
}   // SetupDriverName