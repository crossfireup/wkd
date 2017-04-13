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
#include "ServiceControl.h"
#include "ProcessUtil.h"
#include "PEAnalyze.h"



// The main window class name.
static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("AppControl");

static TCHAR szDriverLocation[1024];

static HINSTANCE hInst;

REGISTER_EVENT registerEvt;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static BOOL InitDriver();

static BOOL DriverIoControl(TCHAR *driverName, int ctrlCode);

static BOOL GetDriverMsg(HANDLE hDevice, REGISTER_EVENT registerEvt);

/* Dialog procedure for pe analyze */
INT_PTR CALLBACK DialogProcToolPE(
	_In_ HWND   hDlg,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	);

INT_PTR CALLBACK AboutDlgProc(HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

static int RegisterCls(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL; // MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			MB_OK);

		return EXIT_SUCCESS;
	}

	return EXIT_SUCCESS;
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HMENU hMenu = NULL;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (RegisterCls(hInstance)){
		return(EXIT_FAILURE);
	}

	hInst = hInstance; // Store instance handle in our global variable

	hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU));

	if (hMenu == NULL){
		MessageBoxPrintf(NULL, MB_OK, "Error", "Load Menu outor: %x\n", GetLastError());
		return EXIT_FAILURE;
	}

	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		600, 500,
		NULL,
		hMenu,
		hInstance,
		NULL
		);

	if (!hWnd)
	{
		MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			MB_OK);

		return EXIT_FAILURE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;

	switch (message)
	{
	case WM_CREATE:

		break;

	case WM_COMMAND:
		hMenu = GetMenu(hWnd);

		switch (LOWORD(wParam)){
		case IDM_FILE_INSTALLDRIVER:
			InitDriver();
			break;

		case IDM_FILE_REMOVEDRIVER:
			ManageDriver(DRIVER_NAME,
				szDriverLocation,
				DRIVER_CTRL_REMOVE
				);
			break;

		case IDM_FILE_STARTMON:
			DriverIoControl(_T("SysMon"), IOCTL_REGISTER_EVENT);
			break;

		case IDM_TOOLS_PEANALYZE:
			if (DialogBox(hInst, MAKEINTRESOURCE(IDD_TOOLS_PROCNAME), hWnd, DialogProcToolPE) == -1){
				DWORD dwError = GetLastError();
				DisplayError(_T(__FUNCTION__"DialogBox"));
			}
			break;

		case IDM_FILE_EXIT:
			PostQuitMessage(0);
			break;

		case IDM_HELP_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);
			break;

		default:
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}

BOOL InitDriver()
{

	if (!SetupDriverName(szDriverLocation, sizeof(szDriverLocation))) {
		return FALSE;
	}

	if (!ManageDriver(DRIVER_NAME,
		szDriverLocation,
		DRIVER_CTRL_INSTALL
		)) {

		MessageBoxPrintf(NULL, MB_OK, "Error", "Unable to install driver. \n");

		ManageDriver(DRIVER_NAME,
			szDriverLocation,
			DRIVER_CTRL_REMOVE
			);

		return TRUE;
	}

	return TRUE;
}


BOOL DriverIoControl(TCHAR *driverName, int ctrlCode)
{
	HANDLE hDevice = NULL;
	TCHAR *devicePath = NULL;
	HRESULT hResult = -1;
	BOOL bRet = TRUE;

	devicePath = (TCHAR *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(TCHAR) * BUF_SIZE);
	if (devicePath == NULL){
		DisplayError(__FUNCTION__);
		return FALSE;
	}

	hResult = StringCbPrintf(devicePath, sizeof(TCHAR) * BUF_SIZE, "\\\\.\\%s", driverName);
	if (FAILED(hResult)){
		DisplayError(_T("StringCbPrintf"));
		bRet = FALSE;
		goto out;
	}

	hDevice = CreateFile(
		devicePath,                     // lpFileName
		GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
		FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
		NULL,                               // lpSecurityAttributes
		OPEN_EXISTING,                      // dwCreationDistribution
		0,                                  // dwFlagsAndAttributes
		NULL                                // hTemplateFile
		);
	if (hDevice == INVALID_HANDLE_VALUE){
		DisplayError(_T(__FUNCTION__"#CreateFile"));
		bRet = FALSE;
		goto out;
	}

	switch (ctrlCode){
	case IOCTL_REGISTER_EVENT:
		GetDriverMsg(hDevice, registerEvt);
		break;

	default:
		MessageBoxPrintf(NULL, MB_OK, "Error", "Control Code %x not implemented yet\n", ctrlCode);
		bRet = FALSE;
		break;
	}

out:
	if (devicePath)
		LocalFree(devicePath);
	if (hDevice)
		CloseHandle(hDevice);
	return bRet;
}

BOOL GetDriverMsg(HANDLE hDevice, REGISTER_EVENT registerEvt)
{
	BOOL bRet = TRUE;
	PTCHAR pOutBuf = NULL;

	registerEvt.Type = EVENT_BASED;
	registerEvt.hEvent = CreateEvent(NULL,
		TRUE,
		FALSE,
#if DBG
		"HookEvent"
#else
		NULL
#endif
		);

	if (registerEvt.hEvent == NULL){
		DisplayError(__FUNCTION__);
		return FALSE;
	}

	pOutBuf = (PTCHAR)LocalAlloc(LPTR, sizeof(TCHAR) * BUF_SIZE);
	if (pOutBuf == NULL){
		DisplayError(_T("LocalAlloc"));
		bRet = FALSE;
		goto out;
	}

	bRet = DeviceIoControl(hDevice,
		IOCTL_REGISTER_EVENT,
		&registerEvt,
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

	WaitForSingleObject(registerEvt.hEvent, INFINITE);

	MessageBoxPrintf(NULL, MB_OK, "Information", _T("Recieve message from Driver: %s"), pOutBuf);

out:
	if (pOutBuf)
		LocalFree(pOutBuf);

	CloseHandle(registerEvt.hEvent);

	return bRet;
}

INT_PTR CALLBACK AboutDlgProc(HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg){
	case WM_INITDIALOG:
	{
		LPTSTR lpBuffer = NULL;

		lpBuffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 512*sizeof(TCHAR));
		if (lpBuffer == NULL){
			DisplayError(_T(__FUNCTION__"#LocalAlloc"));
			return FALSE;
		}
		LoadString(hInst, IDS_VERSION, lpBuffer, 512);
		SetDlgItemText(hDlg, IDC_STATIC_VERSION, lpBuffer);
		LocalFree(lpBuffer);
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)){
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	break;
	}
	
	return FALSE;
}

INT_PTR CALLBACK DialogProcToolPE(
	_In_ HWND   hDlg,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	)
{
	static LPTSTR lpBuffer;
	static const nMaxCount = 512 * sizeof(TCHAR);;

	switch (uMsg){
	case WM_INITDIALOG:

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDC_BT_PN_OK:
		{
			DWORD pid;
			PVOID pImageBase;

			lpBuffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, nMaxCount);
			if (lpBuffer == NULL){
				DisplayError(_T(__FUNCTION__"LocalAlloc"));
				return FALSE;
			}
			memset(lpBuffer, '\0', nMaxCount);

			if (!GetDlgItemText(hDlg, IDC_EDIT_PROCNAME, lpBuffer, nMaxCount))
			{
				DisplayError(_T(__FUNCTION__"GetDlgItemText"));
				LocalFree(lpBuffer);
				return FALSE;
			}

			if (!GetPidByName(lpBuffer, &pid)){
				LocalFree(lpBuffer);
				return FALSE;
			}

			memset(lpBuffer, '\0', nMaxCount);
			if (!GetProcessImageName(pid, lpBuffer)){
				LocalFree(lpBuffer);
				return FALSE;
			}

			pImageBase = GetImageMapView(lpBuffer);
			if (!pImageBase){
				LocalFree(lpBuffer);
				return FALSE;
			}

			GetDosHeader(pImageBase);
			LocalFree(lpBuffer);
			return TRUE;
		}

		case IDCANCEL:
		case IDC_BT_PN_CANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}