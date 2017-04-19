#include "stdafx.h"
#include "ProcessUtil.h"


BOOL GetProcessImageName(DWORD  pid, LPTSTR lpFilename)
{
	HANDLE	hProcess = NULL;
	DWORD	dwSize = 1024 * sizeof(TCHAR);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

	if (hProcess == NULL){
		DisplayError(_T(__FUNCTION__)_T("OpenProcess"));
		return FALSE;
	}

	if (lpFilename == NULL){
		DisplayError(_T(__FUNCTION__)_T("LocalAlloc"));
		CloseHandle(hProcess);
		return FALSE;
	}

	if (!QueryFullProcessImageName(hProcess, 0, lpFilename, &dwSize)){
		DisplayError(_T(__FUNCTION__)_T("GetModuleFileNameEx"));
		CloseHandle(hProcess);
		return FALSE;
	}
	
	return TRUE;
}

BOOL GetPidByName(LPCTSTR lpProcName, DWORD *pid)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	if (lpProcName == NULL){
		return FALSE;
	}

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		DisplayError(_T(__FUNCTION__)_T("CreateToolhelp32Snapshot (of processes)"));
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hProcessSnap, &pe32)){
		DisplayError(_T(__FUNCTION__)_T("Process32First"));
		CloseHandle(hProcessSnap);
		return FALSE;
	}

	do {
		if (StrStr(pe32.szExeFile, lpProcName)){
			*pid = pe32.th32ProcessID;
			return TRUE;
		}
	} while (Process32Next(hProcessSnap, &pe32));

	return FALSE;
}