#ifndef _COMM_H
#define _COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>


#define BUF_SIZE  1024

#ifdef DBG
#define DRIVER_NAME L"HookSSDT"
#else
#define DRIVER_NAME "MSINFO"
#endif

	int MessageBoxPrintf(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR fmt, ...);
	
	void DisplayError(LPCTSTR lpszFunction);

#ifdef __cplusplus
}
#endif

#endif /* _COMM_H */