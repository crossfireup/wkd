#ifndef MAINUI_H
#define MAINUI_H

#include <Windows.h>

class MainUI {
public:
	MainUI();
	~MainUI();

	UINT CreateMainUIAndLoop(HINSTANCE hInstance, wchar_t *cmdline, int nCmdShow, PVOID cls);

private:
	static int RegisterClass(HINSTANCE hInstance);

	static MainUI *FromWindow(HWND hMainWnd);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static bool OnCreate(HWND parentWnd, LPCREATESTRUCT);

	void OnDestroy(HWND hWnd);

	void OnSize(HWND hWnd);

	void OnPaint(HWND hWnd);

	void OnFileExit(HWND hWnd);

	void OnComand(HWND hWnd);	 

	INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT uMsg,	WPARAM wParam, LPARAM lParam);

	/* Dialog procedure for pe analyze */
	INT_PTR CALLBACK DialogProcToolPE(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

#endif MAINUI_H