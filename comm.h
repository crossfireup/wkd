#ifndef _COMM_H
#define _COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>


#define BUF_SIZE  1024
#ifdef DBG
#define DRIVER_NAME "HookSSDT"
#else
#define DRIVER_NAME "MSINFO"
#endif

	int MessageBoxPrintf(HWND hWnd, UINT uType, LPCSTR lpCaption, LPCSTR fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* _COMM_H */