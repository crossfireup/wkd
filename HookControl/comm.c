#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <strsafe.h>
#include "comm.h"


int MessageBoxPrintf(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR fmt, ...)
{
	va_list ap;
	TCHAR  buf[BUF_SIZE];
	int nSize = 0;

	va_start(ap, fmt);

	memset(buf, '\0', BUF_SIZE * sizeof(TCHAR));

	nSize = vsnprintf_s(buf, BUF_SIZE * sizeof(TCHAR), (BUF_SIZE - 1) * sizeof(TCHAR), fmt, ap);
	if (nSize < 0)
		return nSize;

	MessageBox(hWnd, buf, lpCaption, uType);

	va_end(ap);

	return nSize;
}

void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
	LPTSTR lpMsgBuf;
	int nMsgBuf = 0;
	LPTSTR lpDisplayBuf;
	DWORD dwErr = GetLastError();

	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL) == 0){
		MessageBoxPrintf(NULL, MB_OK, "Error", "LocalAlloc failed: %x\n", GetLastError());
		return;
	}

	lpDisplayBuf = (LPTSTR)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen(lpMsgBuf)
		+ lstrlen((LPCTSTR)lpszFunction)
		+ 80) // account for format string
		* sizeof(TCHAR)
		);

	if (lpDisplayBuf == NULL){
		LocalFree(lpMsgBuf);
		MessageBoxPrintf(NULL, MB_OK, "Error", "LocalAlloc failed: %x\n", GetLastError());
		return;
	}


	nMsgBuf = LocalSize(lpDisplayBuf);
	if (nMsgBuf == 0){
		MessageBoxPrintf(NULL, MB_OK, "Error", "LocalSize failed: %x\n", GetLastError());
		return;
	}

	if (FAILED(StringCchPrintf(lpDisplayBuf,
		nMsgBuf,
		_T("%s failed with error code %d as follows:\n%s"),
		lpszFunction,
		dwErr,
		lpMsgBuf)))
	{
		MessageBox(NULL, _T("Unable to output error code.\n"), _T("FATAL ERROR"), MB_OK);
	}
	else{
		MessageBox(NULL, lpDisplayBuf, _T("Error"), MB_OK);
	}

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

	return;
}
