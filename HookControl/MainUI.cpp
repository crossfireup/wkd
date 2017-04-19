#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>

#include "comm.h"
#include "resource.h"
#include "ServiceBase.h"
#include "HookService.h"
#include "ServiceManager.h"
#include "HookControl.h"
#include "..\HookSSDT\public.h"
#include "PEAnalyze.h"
#include "ProcessUtil.h"
#include "MainUI.h"

struct AppData_ {
	ServiceManager *serviceManager;
	ServiceBase	*service;
	HookControl *hookCtrl;
};

static struct AppData_ MainUIAppData;

// The main window class name.
static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("AppControl");

static HINSTANCE hInst;

REGISTER_EVENT registerEvt;

int WINAPI _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine,
	int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance; // Store instance handle in our global variable

	MainUI mainUI;
	mainUI.CreateMainUIAndLoop(hInstance, lpCmdLine, nCmdShow, NULL);
}


MainUI::MainUI()
{

}

MainUI::~MainUI()
{

}

UINT MainUI::CreateMainUIAndLoop(HINSTANCE hInstance, wchar_t *cmdline, int nCmdShow, PVOID cls)
{
	if (!MainUI::RegisterClass(hInstance)){
		return FALSE;
	}

	HMENU hMenu = ::LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));

	if (NULL == hMenu){
		MessageBoxPrintf(NULL, MB_OK, L"Error", L"Load Menu error: %08lx\n", GetLastError());
		return FALSE;
	}

	HWND hWnd = ::CreateWindow(
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
		::MessageBox(NULL,
			_T("Call to CreateWindow failed!"),
			_T("Win32 Guided Tour"),
			MB_OK);

		return FALSE;
	}

	::ShowWindow(hWnd, nCmdShow);
	::UpdateWindow(hWnd);

	// Main message loop:
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
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

	if (!::RegisterClassEx(&wcex))
	{
		::MessageBox(NULL,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			MB_OK);

		return FALSE;
	}

	return TRUE;
}

MainUI* MainUI::FromWindow(HWND hMainWnd)
{
	return reinterpret_cast<MainUI *>(::GetWindowLongPtr(hMainWnd, GWL_USERDATA));
}

bool MainUI::OnCreate(HWND hWnd, LPCREATESTRUCT)
{
	MainUIAppData.service = &HookService(DRIVER_NAME);

	MainUIAppData.serviceManager = &ServiceManager(DRIVER_NAME, DRIVER_NAME);

	if (MainUIAppData.serviceManager->IsInitOk() == FALSE) {
		EnableMenuItem(GetMenu(hWnd), IDM_FILE_INSTALLDRIVER, MF_GRAYED);
		EnableMenuItem(GetMenu(hWnd), IDM_FILE_REMOVEDRIVER, MF_GRAYED);
		EnableMenuItem(GetMenu(hWnd), IDM_FILE_STARTMON, MF_GRAYED);
		EnableMenuItem(GetMenu(hWnd), IDM_FILE_STOPMON, MF_GRAYED);
	}
	
	MainUIAppData.hookCtrl = &HookControl(*MainUIAppData.service, *MainUIAppData.serviceManager);

	::SetWindowLongPtr(hWnd, GWL_USERDATA, reinterpret_cast<INT_PTR>(hWnd));

	return true;
}

void MainUI::OnDestroy(HWND hWnd)
{
	::PostQuitMessage(0);
}

void MainUI::OnSize(HWND hWnd)
{

}

void MainUI::OnPaint(HWND hWnd)
{

}

void MainUI::OnFileExit(HWND hWnd)
{
	OnDestroy(hWnd);
}

void MainUI::OnComand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
	//HMENU hMenu = GetMenu(hWnd);

	switch (id){
	case IDM_FILE_INSTALLDRIVER:
		MainUIAppData.hookCtrl->InitDriver();
		break;

	case IDM_FILE_REMOVEDRIVER:
		MainUIAppData.serviceManager->ServiceControl(
			SERVICE_CTRL_REMOVE
			);
		break;

	case IDM_FILE_STARTMON:
		MainUIAppData.hookCtrl->DriverIoControl(IOCTL_REGISTER_EVENT);
		break;

	case IDM_TOOLS_PEANALYZE:
		if (::DialogBox(hInst, MAKEINTRESOURCE(IDD_TOOLS_PROCNAME), hWnd, DialogProcToolPE) == -1){
			DWORD dwError = ::GetLastError();
			DisplayError(_T(__FUNCTION__)_T("#DialogBox"));
		}
		break;

	case IDM_FILE_EXIT:
		PostQuitMessage(0);
		break;

	case IDM_HELP_ABOUT:
		::DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDlgProc);
		break;

	default:
		break;
	}
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
LRESULT CALLBACK MainUI::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	MainUI* host = FromWindow(hWnd);

	switch (message)
	{
		HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hWnd, WM_COMMAND, host->OnComand);
		HANDLE_MSG(hWnd, WM_DESTROY, host->OnDestroy);

	default:
		break;
	}

	return ::DefWindowProc(hWnd, message, wParam, lParam);
}


INT_PTR CALLBACK MainUI::AboutDlgProc(HWND hDlg,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg){
	case WM_INITDIALOG:
	{
		LPTSTR lpBuffer = NULL;

		lpBuffer = (LPTSTR)::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, 512 * sizeof(TCHAR));
		if (lpBuffer == NULL){
			DisplayError(_T(__FUNCTION__)_T("#LocalAlloc"));
			return FALSE;
		}
		::LoadString(hInst, IDS_VERSION, lpBuffer, 512);
		::SetDlgItemText(hDlg, IDC_STATIC_VERSION, lpBuffer);
		::LocalFree(lpBuffer);
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)){
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, 0);
			return TRUE;
		}
		break;
	}
	break;
	}

	return FALSE;
}

INT_PTR CALLBACK MainUI::DialogProcToolPE(
	_In_ HWND   hDlg,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
	)
{
	static LPWSTR lpBuffer;
	static const int nMaxCount = 512 * sizeof(TCHAR);;

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

			lpBuffer = (LPWSTR)::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, nMaxCount);
			if (lpBuffer == NULL){
				DisplayError(_T(__FUNCTION__)_T("LocalAlloc"));
				return FALSE;
			}
			::memset(lpBuffer, '\0', nMaxCount);

			if (!GetDlgItemText(hDlg, IDC_EDIT_PROCNAME, lpBuffer, nMaxCount))
			{
				DisplayError(_T(__FUNCTION__)_T("GetDlgItemText"));
				::LocalFree(lpBuffer);
				return FALSE;
			}

			if (!GetPidByName(lpBuffer, &pid)){
				::LocalFree(lpBuffer);
				return FALSE;
			}

			memset(lpBuffer, '\0', nMaxCount);
			if (!GetProcessImageName(pid, lpBuffer)){
				::LocalFree(lpBuffer);
				return FALSE;
			}

			pImageBase = GetImageMapView(lpBuffer);
			if (!pImageBase){
				::LocalFree(lpBuffer);
				return FALSE;
			}

			GetDosHeader(pImageBase);
			::LocalFree(lpBuffer);
			return TRUE;
		}

		case IDCANCEL:
		case IDC_BT_PN_CANCEL:
			::EndDialog(hDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}