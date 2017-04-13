#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "comm.h"
#include "resource.h"
#include "HookControl.h"
#include "ServiceControl.h"
#include "..\HookSSDT\public.h"
#include "PEAnalyze.h"
#include "ProcessUtil.h"
#include "MainUI.h"



// The main window class name.
static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("AppControl");

static HINSTANCE hInst;

REGISTER_EVENT registerEvt;

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{


	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance; // Store instance handle in our global variable
}


MainUI::MainUI()
{

}

MainUI::~MainUI()
{

}

UINT MainUI::CreateMainUIAndLoop(HINSTANCE hInstance, wchar_t *cmdline, int nCmdShow, PVOID cls)
{
	if (RegisterClass(hInstance)){
		return FALSE;
	}

	HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));

	if (NULL == hMenu){
		MessageBoxPrintf(NULL, MB_OK, "Error", "Load Menu outor: %x\n", GetLastError());
		return FALSE;
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

		return FALSE;
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

int MainUI::RegisterClass(HINSTANCE hInstance)
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

		return FALSE;
	}

	return TRUE;
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


INT_PTR CALLBACK AboutDlgProc(HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg){
	case WM_INITDIALOG:
	{
		LPTSTR lpBuffer = NULL;

		lpBuffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 512 * sizeof(TCHAR));
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