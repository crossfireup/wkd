/* control driver in kernel mode
 * start and stop driver
 * register event and get message from kernel driver
 */
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>

#include "comm.h"
#include "resource.h"
#include "..\HookSSDT\public.h"
#include "HookControl.h"
#include "ServiceManager.h"
#include "ProcessUtil.h"
#include "PEAnalyze.h"


HookControl::HookControl(ServiceBase& service, ServiceManager& serviceManager)
	:service_{ service }, serviceManager_{serviceManager}
{
}

HookControl::~HookControl()
{
}


BOOL DriverIoControl(int ctrlCode);

BOOL GetDriverMsg();

BOOL HookControl::InitDriver()
{

	if (!serviceManager_.ServiceControl(SERVICE_CTRL_INSTALL
		)) {

		MessageBoxPrintf(NULL, MB_OK, L"Error", L"Unable to install driver. \n");

		serviceManager_.ServiceControl(SERVICE_CTRL_REMOVE);

		return TRUE;
	}

	return TRUE;
}


BOOL HookControl::DriverIoControl(int ctrlCode)
{
	WCHAR *devicePath = NULL;
	HRESULT hResult = -1;
	BOOL bRet = TRUE;

	devicePath = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(TCHAR) * BUF_SIZE);
	if (devicePath == NULL){
		DisplayError(L"DriverIoControl");
		return FALSE;
	}

	hResult = StringCbPrintf(devicePath, sizeof(TCHAR) * BUF_SIZE, L"\\\\.\\%s", DRIVER_NAME);
	if (FAILED(hResult)){
		DisplayError(_T("StringCbPrintf"));
		bRet = FALSE;
		goto out;
	}

	hDevice_ = CreateFile(
		devicePath,                     // lpFileName
		GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
		FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
		NULL,                               // lpSecurityAttributes
		OPEN_EXISTING,                      // dwCreationDistribution
		0,                                  // dwFlagsAndAttributes
		NULL                                // hTemplateFile
		);
	if (hDevice_ == INVALID_HANDLE_VALUE){
		DisplayError(_T(__FUNCTION__"#CreateFile"));
		bRet = FALSE;
		goto out;
	}

	switch (ctrlCode){
	case IOCTL_REGISTER_EVENT:
		GetDriverMsg();
		break;

	default:
		MessageBoxPrintf(NULL, MB_OK, L"Error", L"Control Code %x not implemented yet\n", ctrlCode);
		bRet = FALSE;
		break;
	}

out:
	if (devicePath)
		LocalFree(devicePath);
	if (hDevice_)
		CloseHandle(hDevice_);
	return bRet;
}

BOOL HookControl::GetDriverMsg()
{
	BOOL bRet = TRUE;
	PTCHAR pOutBuf = NULL;

	registerEvt_.Type = EVENT_BASED;
	registerEvt_.hEvent = CreateEvent(NULL,
		TRUE,
		FALSE,
#if DBG
		L"HookEvent"
#else
		NULL
#endif
		);

	if (registerEvt_.hEvent == NULL){
		DisplayError(_T(__FUNCSIG__));
		return FALSE;
	}

	pOutBuf = (PTCHAR)LocalAlloc(LPTR, sizeof(TCHAR) * BUF_SIZE);
	if (pOutBuf == NULL){
		DisplayError(_T("LocalAlloc"));
		bRet = FALSE;
		goto out;
	}

	bRet = DeviceIoControl(hDevice_,
		IOCTL_REGISTER_EVENT,
		&registerEvt_,
		sizeof(REGISTER_EVENT),
		pOutBuf,
		sizeof(TCHAR) * BUF_SIZE,
		NULL,
		NULL
		);
	if (bRet == FALSE){
		DisplayError(_T("DeviceIoControl"));
		goto out;
	}

	WaitForSingleObject(registerEvt_.hEvent, INFINITE);

	MessageBoxPrintf(NULL, MB_OK, L"Information", _T("Recieve message from Driver: %s"), pOutBuf);

out:
	if (pOutBuf)
		LocalFree(pOutBuf);

	CloseHandle(registerEvt_.hEvent);

	return bRet;
}
