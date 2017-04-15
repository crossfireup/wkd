#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include "comm.h"
#include "PEAnalyze.h"

#pragma comment(lib, "Shlwapi")

namespace win{
	HMODULE PEImg::GetModule() const
	{
		return hModule_;
	}

	void PEImg::SetModule(const HMODULE hModule)
	{
		hModule_ = hModule;
	}

	PIMAGE_DOS_HEADER PEImg::GetDosHeader() const
	{
		return reinterpret_cast<PIMAGE_DOS_HEADER>(hModule_);
	}

	PIMAGE_NT_HEADERS PEImg::GetNTHeaders() const
	{
		PIMAGE_DOS_HEADER pDosHeader = GetDosHeader();

		return reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<char *>(pDosHeader) + pDosHeader->e_lfanew);
	}

	WORD PEImg::GetSectionNum() const 
	{
		return GetNTHeaders()->FileHeader.NumberOfSections;
	}

	PIMAGE_SECTION_HEADER PEImg::GetFirstSection() const
	{
		PIMAGE_NT_HEADERS pNtHeaders = GetNTHeaders();

		return reinterpret_cast<PIMAGE_SECTION_HEADER>(pNtHeaders + \
			FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + pNtHeaders->OptionalHeader.SizeOfHeaders);
	}

	PIMAGE_SECTION_HEADER PEImg::GetSectionHeader(UINT section) const
	{
		PIMAGE_NT_HEADERS pNtHeaders = GetNTHeaders();

		PIMAGE_SECTION_HEADER pFirstSection = IMAGE_FIRST_SECTION(pNtHeaders);
		if (section < pNtHeaders->FileHeader.NumberOfSections)
			return pFirstSection + section;
		else
			return NULL;
	}

	PIMAGE_SECTION_HEADER PEImg::GetImageSectionFromAddr(PVOID addr) const
	{
		PBYTE target = reinterpret_cast<BYTE *>(addr);
		PIMAGE_SECTION_HEADER pSection;

		for (int i = 0; NULL != (pSection = GetSectionHeader(i)); i++){
			PBYTE start = reinterpret_cast<PBYTE>(PEImg::RVAToAddr(pSection->VirtualAddress));

			if ((start <= target) && (target <= (start + pSection->Misc.VirtualSize))){
				return pSection;
			}
		}

		return NULL;
	}

	DWORD PEImg::GetImageDirectoryEntrySize(UINT directory) const
	{
		PIMAGE_NT_HEADERS pNtHeaders = GetNTHeaders();

		return pNtHeaders->OptionalHeader.DataDirectory[directory].Size;
	}

	PVOID PEImg::GetImageDirectoryEntryAddr(UINT directory) const
	{
		PIMAGE_NT_HEADERS pNtHeaders = GetNTHeaders();

		return RVAToAddr(pNtHeaders->OptionalHeader.DataDirectory[directory].VirtualAddress);
	}

	PVOID PEImg::RVAToAddr(DWORD rva) const
	{
		if (rva == 0)
			return NULL;

		return reinterpret_cast<char *> (hModule_+ rva);
	}

	PVOID PEImgAsData::RVAToAddr(DWORD rva) const
	{
		if (rva == 0)
			return NULL;

		PVOID addr = PEImg::RVAToAddr(rva);
		DWORD diskOffset;

		if (!ImgAddrToOnDiskOffset(addr, &diskOffset))
			return NULL;

		return PEImg::RVAToAddr(diskOffset);
	}

	bool PEImg::ImgAddrToOnDiskOffset(DWORD rva, DWORD *onDiskOffset) const
	{
		PVOID addr = RVAToAddr(rva);

		return ImgAddrToOnDiskOffset(addr, onDiskOffset);
	}

	bool PEImg::ImgAddrToOnDiskOffset(PVOID addr, DWORD *onDiskOffset) const
	{
		if (NULL == addr)
			return false;

		PIMAGE_SECTION_HEADER pSection = GetImageSectionFromAddr(addr);

		if (NULL == pSection)
			return false;

		DWORD offsetInSection = static_cast<DWORD>(reinterpret_cast<uintptr_t>(addr)) \
			- static_cast<DWORD>(reinterpret_cast<uintptr_t>(PEImg::RVAToAddr(pSection->VirtualAddress)));

		*onDiskOffset = pSection->PointerToRawData + offsetInSection;

		return true;
	}
}

BOOL GetDosHeader(PVOID pvFileBase){
	PIMAGE_DOS_HEADER pImgDosHdr = NULL;

	if (pvFileBase == NULL)
		return FALSE;

	pImgDosHdr = (PIMAGE_DOS_HEADER)pvFileBase;

	if (pImgDosHdr->e_magic == IMAGE_DOS_SIGNATURE){
		MessageBoxPrintf(NULL, MB_OK, L"Information", L"MZ:%x\n", pImgDosHdr->e_magic);
	}
	else {
		MessageBoxPrintf(NULL, MB_OK, L"Information", L"Magic:%x\n", pImgDosHdr->e_magic);
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
		DisplayError(_T(__FUNCTION__)_T("PathFileExists"));
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
		DisplayError(_T(__FUNCTION__)_T("#WriteFile"));
		return hFile;
	}

	if (!GetFileSizeEx(hFile, &lFileSize)){
		DisplayError(_T(__FUNCTION__)_T("#GetFileSizeEx"));
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