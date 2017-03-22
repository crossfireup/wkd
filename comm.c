#include <stdio.h>
#include <stdarg.h>
#include "comm.h"


int MessageBoxPrintf(HWND hWnd, UINT uType, LPCSTR lpCaption, LPCSTR fmt, ...)
{
	va_list ap;
	char buf[BUF_SIZE];
	int nSize = 0;

	va_start(ap, fmt);
	memset(buf, '\0', BUF_SIZE);
	nSize = vsnprintf_s(buf, BUF_SIZE, BUF_SIZE - 1, fmt, ap);
	if (nSize < 0)
		return nSize;
	MessageBox(hWnd, buf, lpCaption, uType);

	va_end(ap);

	return nSize;
}