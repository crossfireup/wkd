#include "HookOps.h"
#include "HookSSDT.h"

HOOK_INFO DisableWP_MDL(DWORD32 *ssdt, DWORD32 nServices)
{
	HOOK_INFO HookInfo;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! original address of SSDT=%x\n", (DWORD32)ssdt);
	HookInfo.pMdl = MmCreateMdl(NULL, (PVOID)ssdt, (SIZE_T)nServices * 4);
	if (HookInfo.pMdl == NULL){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DisableWP_MDL call MmCreateMdl() failed\n");
		return(HookInfo);
	}

	/* update MDL to descripte the underlying physical pages of SSDT */
	MmBuildMdlForNonPagedPool(HookInfo.pMdl);

	/* change flags so that we can perform modifications */
	(*(HookInfo.pMdl)).MdlFlags = (*(HookInfo.pMdl)).MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

	HookInfo.callTable = (DWORD32 *)MmMapLockedPages(HookInfo.pMdl, KernelMode);
	if (HookInfo.callTable == NULL){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DisableWP_MDL call MmMapLockedPages() failed\n");
		return(HookInfo);
	}

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "DisableWP_MDL address of call table: %x\n", (DWORD32)HookInfo.callTable);

	return(HookInfo);
}

void EnableWP_MDL(PMDL pMdl, BYTE *callTable)
{
	if (pMdl != NULL){
		MmUnmapLockedPages((PVOID)callTable, pMdl);
		IoFreeMdl(pMdl);
	}

	return;
}

void DisableCR0WP()
{
	/* clear WP bit, 0xfffeffff:[1111 1111 1111 1111 1110 1111 1111 1111]*/
	__asm
	{
		push ebx;
		mov ebx, cr0;
		and ebx, 0xfffeffff;
		mov cr0, ebx;
		pop ebx;
	}
	return;
}

void EnableCROWP()
{
	/* set WP bit, 0xfffeffff:[0000 0000 0000 0001 0000 0000 0000 0000]*/
	__asm
	{
		push ebx;
		mov ebx, cr0;
		or ebx, 0x00010000;
		mov cr0, ebx;
		pop ebx;
	}
	return;
}

/*
1: kd> u nt!ZwCreateFile
nt!ZwCreateFile:
81885ebc b842000000      mov     eax,42h
81885ec1 8d542404        lea     edx,[esp+4]
81885ec5 9c              pushfd
81885ec6 6a08            push    8
81885ec8 e861230000      call    nt!KiSystemService (8188822e)
81885ecd c22c00          ret     2Ch
nt!ZwCreateIoCompletion:
81885ed0 b843000000      mov     eax,43h
81885ed5 8d542404        lea     edx,[esp+4]

the index is one byte after the function address;
*/
DWORD32 GetSSDTIndex(BYTE *addr)
{
	BYTE *addrOfIndex;
	DWORD32 indexValue;

	addrOfIndex = addr + 1;
	indexValue = *((PULONG)addrOfIndex);
	return indexValue;
}

BYTE *HookSSDT(BYTE *apiCall, BYTE *newAddr, DWORD32 *callTable)
{
	PLONG targetAddr;
	DWORD32 indexValue;

	indexValue = GetSSDTIndex(apiCall);
	targetAddr = (PLONG)&(callTable[indexValue]);

	return((BYTE *)InterlockedExchange(targetAddr, (LONG)newAddr));
}

void UnHookSSDT(BYTE *apiCall, BYTE *oldAddr, DWORD32 *callTable)
{
	PLONG targetAddr;
	DWORD32 indexValue;

	indexValue = GetSSDTIndex(apiCall);
	targetAddr = (PLONG)&(callTable[indexValue]);
	InterlockedExchange(targetAddr, (LONG)oldAddr);

	return;
}

NTSTATUS HookedZwSetValueKey(
	_In_     HANDLE          KeyHandle,
	_In_     PUNICODE_STRING ValueName,
	_In_opt_ ULONG           TitleIndex,
	_In_     ULONG           Type,
	_In_opt_ PVOID           Data,
	_In_     ULONG           DataSize
	)
{
	NTSTATUS status;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey call to set registry value intercepted");

	/* get information of process calling ZwSetValueKey */
	GetProcInfo(PsGetCurrentProcessId());

	HookSSDTGetRegInfo(KeyHandle, *ValueName);

	/* log reg value type */
	switch (Type)
	{
	case(REG_BINARY) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_BINARY\n");
		break;
	case(REG_DWORD) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_BINARY\n");
		break;
	case(REG_EXPAND_SZ) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_EXPAND_SZ\n");
		break;
	case(REG_LINK) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_LINK\n");
		break;
	case(REG_MULTI_SZ) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_MULTI_SZ\n");
		break;
	case(REG_NONE) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_NONE\n");
		break;
	case(REG_SZ) :
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=REG_SZ\n");
		break;
	default:
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t Type=Unknown\n");
	}

	/* call orignal function */
	status = ((ZwSetValueKeyPtr)(oldZwSetValueKey))(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);

	return(status);
}


NTSTATUS HookSSDTGetRegInfo(HANDLE keyHandle, UNICODE_STRING regValueName)
{
	NTSTATUS status;
	ANSI_STRING ansiValueName = { 0 };

	PAGED_CODE();

	/* get regpath info */
	status = RtlUnicodeStringToAnsiString(&ansiValueName, &regValueName, TRUE);
	if (NT_SUCCESS(status)){
		ULONG keyInfoLength = 0L;
		PKEY_BASIC_INFORMATION pKeyBasicInfo = NULL;

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t ValueName=%s\n", ansiValueName.Buffer);
		RtlFreeAnsiString(&ansiValueName);

		pKeyBasicInfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEY_BASIC_INFORMATION) + 128, POOLTAG_HOOK_SSDT_01);
		if (pKeyBasicInfo == NULL){
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HookedZwSetValueKey: call ExAllocatPoolWithTag() error!");
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		status = ZwQueryKey(keyHandle, KeyBasicInformation, pKeyBasicInfo, sizeof(KEY_BASIC_INFORMATION) + 128, &keyInfoLength);

		if (!NT_SUCCESS(status)){
			if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL){
				ExFreePoolWithTag(pKeyBasicInfo, POOLTAG_HOOK_SSDT_01);
				pKeyBasicInfo = ExAllocatePoolWithTag(NonPagedPoolNx, keyInfoLength, POOLTAG_HOOK_SSDT_01);
				if (pKeyBasicInfo == NULL){
					TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HookedZwSetValueKey: call ExAllocatPoolWithTag() error!");
					return STATUS_INSUFFICIENT_RESOURCES;;
				}
				else{
					status = ZwQueryKey(keyHandle, KeyBasicInformation, pKeyBasicInfo, keyInfoLength, &keyInfoLength);
				}
			}
			else
			{
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
					"HookedZwSetValueKey::\t call ZwQueryKey() failed, errorCode=%x\n", status);
				return status;
			}
		}

		if (NT_SUCCESS(status)) {
			WCHAR *regPath;
			UNICODE_STRING uniRegPath = { 0 };
			ANSI_STRING ansiRegPath = { 0 };

			regPath = ExAllocatePoolWithTag(NonPagedPool, (pKeyBasicInfo->NameLength + 1) * sizeof(WCHAR), POOLTAG_HOOK_SSDT_02);
			if (regPath == NULL){
				TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "HookedZwSetValueKey: call ExAllocatPoolWithTag() error!");
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			RtlZeroMemory(regPath, pKeyBasicInfo->NameLength + 1);
			RtlCopyMemory(regPath, pKeyBasicInfo->Name, pKeyBasicInfo->NameLength);

			RtlInitUnicodeString(&uniRegPath, regPath);
			RtlUnicodeStringToAnsiString(&ansiRegPath, &uniRegPath, TRUE);
			TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "HookedZwSetValueKey :\t RegPath=%s, length=%d\n",
				ansiRegPath.Buffer, pKeyBasicInfo->NameLength);
			ExFreePoolWithTag(pKeyBasicInfo, POOLTAG_HOOK_SSDT_01);
			ExFreePoolWithTag(regPath, POOLTAG_HOOK_SSDT_02);
			RtlFreeAnsiString(&ansiRegPath);
		}
	}

	return STATUS_SUCCESS;
}

// __declspec(dllimport) DWORD32 PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* pProcess);
__declspec(dllimport) NTSTATUS ZwOpenProcess(
	_Out_    PHANDLE            ProcessHandle,
	_In_     ACCESS_MASK        DesiredAccess,
	_In_     POBJECT_ATTRIBUTES ObjectAttributes,
	_In_opt_ PCLIENT_ID         ClientId
	);


__declspec(dllimport) BYTE *PsGetProcessPeb(PEPROCESS Process);
/*__declspec(dllimport)*/
typedef NTSTATUS(*ZwQueryInformationProcessPtr) (
	_In_      HANDLE           ProcessHandle,
	_In_      PROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
	);



NTSTATUS GetProcInfo(HANDLE ProcessId)
{
	NTSTATUS status;
	HANDLE hProcess;
	OBJECT_ATTRIBUTES objectAttr;
	CLIENT_ID clientID;
	ZwQueryInformationProcessPtr ZwQueryInformationProcess = NULL;

	PAGED_CODE();

	InitializeObjectAttributes(&objectAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	clientID.UniqueProcess = ProcessId;
	clientID.UniqueThread = 0;

	status = ZwOpenProcess(&hProcess, READ_CONTROL, &objectAttr, &clientID);
	if (!NT_SUCCESS(status)){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "failed, errorCode=%d\n", status);
		return(status);
	}

	UNICODE_STRING uniProcName;
	RtlInitUnicodeString(&uniProcName, L"ZwQueryInformationProcess");

	ZwQueryInformationProcess = (ZwQueryInformationProcessPtr)MmGetSystemRoutineAddress(&uniProcName);
	if (NULL == ZwQueryInformationProcess){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "failed, errorCode=%d\n", status);
		return STATUS_PROCEDURE_NOT_FOUND;
	}


	PVOID imgFileNameBuf = NULL;
	ULONG nameLen = 0;

	status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, NULL, 0, &nameLen);
	if (status != STATUS_INFO_LENGTH_MISMATCH){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "failed, errorCode=%d\n", status);
		return status;
	}

	imgFileNameBuf = ExAllocatePoolWithTag(NonPagedPool, nameLen, POOLTAG_HOOK_SSDT_01);
	if (imgFileNameBuf == NULL){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ExAllocatePoolWithTag failed, errorCode=%d\n", status);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(imgFileNameBuf, nameLen);
	status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		imgFileNameBuf,
		nameLen,
		&nameLen);
	if (!NT_SUCCESS(status)){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "get process info fialed, errorCode=%d\n", status);
		ExFreePoolWithTag(imgFileNameBuf, POOLTAG_HOOK_SSDT_01);
		return(status);
	}

	PUNICODE_STRING pUniImgFileName = (PUNICODE_STRING)imgFileNameBuf;

	if (pUniImgFileName->Length > 0)
	{
		ANSI_STRING imgFileName;
		RtlUnicodeStringToAnsiString(&imgFileName, pUniImgFileName, TRUE);
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "PID: %d, ImageFilePath: %s\n", ProcessId, imgFileName.Buffer);
		RtlFreeAnsiString(&imgFileName);
		ExFreePoolWithTag(imgFileNameBuf, POOLTAG_HOOK_SSDT_01);
		return STATUS_SUCCESS;
	}
	else {
		return STATUS_UNSUCCESSFUL;
	}

	//LIST_ENTRY listNode = processBasicInfo.PebBaseAddress->LoaderData->InMemoryOrderModuleList;

	/*ANSI_STRING ansiCmdLine;
	status = RtlUnicodeStringToAnsiString(&ansiCmdLine, &processBasicInfo.PebBaseAddress->ProcessParameters->CommandLine, TRUE);
	if (NT_SUCCESS(status)){
	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Caller Process commandline:%s\n", ansiCmdLine.Buffer);
	RtlFreeAnsiString(&ansiCmdLine);
	}
	else{
	TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "GetProcInfo() call RtlUnicodeStringToAnsiString failed, errorCode=%d\n", status);
	}

	return(status);*/
}

VOID
ProcNotifyRoutine(
IN HANDLE  ParentId,
IN HANDLE  ProcessId,
IN BOOLEAN  Create
)
{
	if (Create){
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "new Process Created, PPID:%d, PID:%d\n", ParentId, ProcessId);
	}
	else {
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "new Process exit, PPID:%d, PID:%d\n", ParentId, ProcessId);
	}
	GetProcInfo(ParentId);
	GetProcInfo(ProcessId);

	return;
}


//PVOID GetProcAddr(UNICODE_STRING modeName, UNICODE_STRING procName)
//{
//	HINSTANCE hinstLib;
//	MYPROC ProcAdd;
//	BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
//
//	// Get a handle to the DLL module.
//
//	hinstLib = LoadLibrary(TEXT("MyPuts.dll"));
//
//	// If the handle is valid, try to get the function address.
//
//	if (hinstLib != NULL)
//	{
//		ProcAdd = (MYPROC)GetProcAddress(hinstLib, "myPuts");
//
//		// If the function address is valid, call the function.
//
//		if (NULL != ProcAdd)
//		{
//			fRunTimeLinkSuccess = TRUE;
//			(ProcAdd)(L"Message sent to the DLL function\n");
//		}
//		// Free the DLL module.
//
//		fFreeResult = FreeLibrary(hinstLib);
//	}
//
//	// If unable to call the DLL function, use an alternative.
//	if (!fRunTimeLinkSuccess)
//		printf("Message printed from executable\n");
//
//	return 0;
//
//}
