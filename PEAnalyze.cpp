#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include "comm.h"
#include "PEAnalyze.h"

#pragma comment(lib, "Shlwapi")

BOOL GetDosHeader(PVOID pvFileBase){
	PIMAGE_DOS_HEADER pImgDosHdr = NULL;

	if (pvFileBase == NULL)
		return FALSE;

	pImgDosHdr = (PIMAGE_DOS_HEADER)pvFileBase;

	if (pImgDosHdr->e_magic == IMAGE_DOS_SIGNATURE){
		MessageBoxPrintf(NULL, MB_OK, "Information", "MZ:%x\n", pImgDosHdr->e_magic);
	}
	else {
		MessageBoxPrintf(NULL, MB_OK, "Information", "Magic:%x\n", pImgDosHdr->e_magic);
	}

	return TRUE;
}

PVOID GetImageMapView(LPCTSTR szFileName)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hFileMap = NULL;
	LARGE_INTEGER lFileSize = { 0 };
	PVOID pvFileBase = NULL;

	if (!PathFileExists(szFileName)){
		DisplayError(_T(__FUNCTION__"PathFileExists"));
		return INVALID_HANDLE_VALUE;
	}

	hFile = CreateFile(szFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE){
		DisplayError(_T(__FUNCTION__"WriteFile"));
		return hFile;
	}

	if (!GetFileSizeEx(hFile, &lFileSize)){
		DisplayError(_T(__FUNCTION__"GetFileSizeEx"));
		goto out;
	}

	hFileMap = CreateFileMapping(hFile,
		NULL,
		PAGE_READONLY | SEC_IMAGE,
		lFileSize.HighPart,
		lFileSize.LowPart,
		NULL);

	if (hFileMap == NULL){
		DisplayError(_T(__FUNCTION__"CreateFileMapping"));
		goto out;
	}

	pvFileBase = MapViewOfFileEx(hFileMap,
		FILE_MAP_READ,
		0,
		0,
		lFileSize.QuadPart,
		NULL);

out:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	if (hFileMap != NULL)
		CloseHandle(hFileMap);

	return pvFileBase;
}