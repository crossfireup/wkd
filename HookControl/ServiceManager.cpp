#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <Shlwapi.h>

#include "comm.h"
#include "ServiceManager.h"
#include "HookService.h"

SC_HANDLE ServiceManager::hSCM_ = NULL;

ServiceManager::ServiceManager(LPTSTR imgPath, LPTSTR serviceName)
{
	if (NULL == hSCM_){
		hSCM_ = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (NULL == hSCM_){
			DisplayError(L"OpenSCManager");
			initOk_ = FALSE;
			return;
		}

	}

	if (NULL == imgPath || NULL == serviceName){
		initOk_ = FALSE;
		return;
	}

	if (!SetImagePath(imgPath)){
		initOk_ = FALSE;
		return;
	}

	if (!SetServiceName(imgPath)){
		initOk_ = FALSE;
		return;
	}

}

ServiceManager::~ServiceManager()
{

}

int ServiceManager::InstallService()
{
	SC_HANDLE hService = NULL;
	DWORD dwErr = 0;

	hService = CreateService(hSCM_,
		serviceName_,
		serviceName_,
		SC_MANAGER_CREATE_SERVICE,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		imgPath_,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
		);

	if (NULL == hService){
		dwErr = GetLastError();

		if (ERROR_SERVICE_EXISTS == dwErr){
			MessageBoxPrintf(NULL, MB_OK, L"Information", L"Service has installed already");
			return TRUE;
		}
		else if (ERROR_SERVICE_MARKED_FOR_DELETE == dwErr){
			MessageBoxPrintf(NULL, MB_OK, L"Information", L"Service marked for delete");
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

int ServiceManager::BeginService()
{
	SC_HANDLE  schService;
	int retCode = 0;
	DWORD dwErr = 0;

	schService = OpenService(hSCM_,
		serviceName_,
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
			DisplayError(L"StartService");
		}
	}

	CloseServiceHandle(schService);

	return retCode;
}

int ServiceManager::StopService()
{
	SC_HANDLE       schService = NULL;
	int				retCode = 0;
	SERVICE_STATUS  serviceStatus = { 0 };

	schService = OpenService(hSCM_,
		serviceName_,
		SERVICE_STOP
		);

	if (schService == NULL) {

		DisplayError(L"OpenService");
		return FALSE;
	}

	retCode = ControlService(schService,
		SERVICE_CONTROL_STOP,
		&serviceStatus
		);
	if (!retCode) {
		DisplayError(L"ControlServic");
	}

	CloseServiceHandle(schService);

	return retCode;
} //  StopService


int ServiceManager::RemoveService()
{
	SC_HANDLE hService = NULL;
	DWORD dwErr = 0;
	int retCode = 0;

	hService = OpenService(hSCM_,
		serviceName_,
		SERVICE_STOP | DELETE);

	if (NULL == hService){
		DisplayError(_T("OpenService"));
		return FALSE;
	}

	retCode = DeleteService(hService);

	if (retCode == FALSE){
		dwErr = GetLastError();
		if (dwErr == ERROR_SERVICE_MARKED_FOR_DELETE){
			MessageBoxPrintf(NULL, MB_OK, L"Error", L"The specified service has already been marked for deletion");
		}
		else {
			DisplayError(_T("DeleteService"));
		}
	}

	CloseServiceHandle(hService);

	return retCode;
}


int ServiceManager::ServiceControl(int ctrlCode)
{
	SC_HANDLE schSCManager = NULL;
	int retCode = TRUE;
	DWORD dwErr = 0;

	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_CREATE_SERVICE
		);

	if (schSCManager == NULL){
		DisplayError(L"OpenSCManager");
		return FALSE;
	}

	switch (ctrlCode){
	case SERVICE_CTRL_INSTALL:
		if (InstallService()){
			BeginService();
		}
		break;

	case SERVICE_CTRL_REMOVE:
		StopService();
		RemoveService();
		break;

	default:
	{
		SC_HANDLE schService;
		SERVICE_STATUS serviceStatus = { 0 };

		schService = OpenService(hSCM_,
			serviceName_,
			SERVICE_STOP
			);

		if (schService == NULL) {
			DisplayError(L"OpenService");
			retCode = FALSE;
			break;
		}
		if (!ControlService(schService, ctrlCode, &serviceStatus)){
			DisplayError(L"ServiceContro");
			retCode = FALSE;
		}
		CloseServiceHandle(schService);
		retCode = TRUE;
	}
	break;
	}

	CloseServiceHandle(schSCManager);

	return(retCode);
}

BOOL ServiceManager::SetImagePath(LPCWSTR imgPath)
{
	LPWSTR imgName = NULL;
	WCHAR bufferPath[MAX_PATH];
	SIZE_T nLen = 0;

	if (imgPath == NULL || 0 == wcslen(imgPath)){
		return FALSE;
	}

	if (PathFileExists(imgPath)){
		nLen = wcslen(imgPath) + 1;
		imgPath_ = new WCHAR[nLen];
		StringCchCopy(imgPath_, nLen, imgPath);

		return TRUE;
	}

	/* find image in current directory */
	nLen = GetCurrentDirectory(MAX_PATH, bufferPath);

	if (nLen == 0){
		DisplayError(L"GetCurrentDirectory");
		return FALSE;
	}

	if (nLen > MAX_PATH){
		MessageBoxPrintf(NULL, MB_OK, L"Error", L"Buffer too small, need %d characters\n", nLen);
		return FALSE;
	}

	if (FAILED(StringCchCat(bufferPath, MAX_PATH, imgPath))){
		DisplayError(_T("StringCbCat"));
		return FALSE;
	}

	if (!PathFileExists(bufferPath)) {
		MessageBoxPrintf(NULL, MB_OK, L"Error", L"%s is not found.\n", imgPath);
		return FALSE;
	}

	nLen = wcslen(bufferPath) + 1;
	imgPath_ = new WCHAR[];
	StringCchCopy(imgPath_, nLen, bufferPath);

	return TRUE;
}

BOOL ServiceManager::SetServiceName(LPCWSTR serviceName)
{
	SIZE_T nLen = 0;

	if (serviceName == NULL || (nLen = wcslen(serviceName)) == 0)
		return FALSE;

	serviceName_ = new WCHAR[nLen + 1];
	StringCchCopy(serviceName_, nLen + 1, serviceName);

	return TRUE;
}
