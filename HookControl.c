/* control driver in kernel mode
 * start and stop driver
 * register event and get message from kernel driver
 */
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#include "comm.h"
#include "resource.h"
#include "HookControl.h"
#include "ServiceControl.h"



// The main window class name.
static TCHAR szWindowClass[] = _T("win32app");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Win32 Guided Tour Application");

static TCHAR szDriverLocation[1024];

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static int InitDriver();

static int RegisterCls(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL; // MAKEINTRESOURCE(IDR_MENU);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

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
		MessageBoxPrintf(NULL, MB_OK, "Error", "Load Menu error: %x\n", GetLastError());
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

		case IDM_FILE_EXIT:
			PostQuitMessage(0);
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

int InitDriver()
{
	HANDLE hDevice = NULL;

	if (!SetupDriverName(szDriverLocation, sizeof(szDriverLocation))) {
		return 0;
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

		return 1;
	}

	//
	// Now open the device again.
	//
	hDevice = CreateFile(
		"\\\\.\\Device\\SysMon",                     // lpFileName
		GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
		FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
		NULL,                               // lpSecurityAttributes
		OPEN_EXISTING,                      // dwCreationDistribution
		0,                                  // dwFlagsAndAttributes
		NULL                                // hTemplateFile
		);

	if (hDevice == INVALID_HANDLE_VALUE){
		MessageBoxPrintf(NULL, MB_OK, "Error", "CreatFile Failed : %d\n", GetLastError());
		return 0;
	}

	return 1;
}