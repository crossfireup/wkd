#ifndef _PROCESSUTIL_H
#define _PROCESSUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>

	BOOL GetProcessImageName(DWORD  pid, LPTSTR lpFilename);
	BOOL GetPidByName(LPCTSTR lpProcName, DWORD *pid);

#ifdef __cplusplus
}
#endif

#endif /* _PROCESSUTIL_H */