#ifndef _HOOKOPS_H
#define _HOOKOPS_H

#include <ntddk.h>
#include <ntstrsafe.h>
#include "HookSSDT.h"

#ifdef __cplusplus
extern "C" {
#endif

	/*
	* hook data struct and functions
	*/
	typedef unsigned char       BYTE;

#pragma pack(1)
	typedef struct _KSERVICE_DESCRIPTOR_ENTRY
	{
		DWORD32 *KiServiceTable;			/* address of SSDT */
		DWORD32 *ServiceCounterTableBase;	/* not used*/
		DWORD32 nSyscalls;					/* number of system calls */
		DWORD32 *ParamTableBase;			/* byte array */
	}KSERVICE_DESCRIPTOR_ENTRY, *PKSERVICE_DESCRIPTOR_ENTRY;
#pragma pack()

	typedef struct _KSERVICE_DESCRIPTOR_TABLE
	{
		KSERVICE_DESCRIPTOR_ENTRY ntoskrnl;		/* SSDT */
		KSERVICE_DESCRIPTOR_ENTRY win32k;		/* Shadow SSDT */
		KSERVICE_DESCRIPTOR_ENTRY table3;		/* empty */
		KSERVICE_DESCRIPTOR_ENTRY table4;		/* empty */
	}KSERVICE_DESCRIPTOR_TABLE, *PKSERVICE_DESCRIPTOR_TABLE;

	typedef struct _HOOK_INFO
	{
		DWORD32 *callTable;	/* address of SSDT mapped to new memory region */
		PMDL pMdl;			/* pointer to MDL */
	}HOOK_INFO;

	
	typedef struct _PEB_LDR_DATA {
		BYTE       Reserved1[8];
		PVOID      Reserved2[3];
		LIST_ENTRY InMemoryOrderModuleList;
	} PEB_LDR_DATA, *PPEB_LDR_DATA;

	typedef struct _RTL_USER_PROCESS_PARAMETERS {
		BYTE           Reserved1[16];
		PVOID          Reserved2[10];
		UNICODE_STRING ImagePathName;
		UNICODE_STRING CommandLine;
	} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

	typedef VOID(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

	typedef struct _PEB {
		BYTE                          Reserved1[2];
		BYTE                          BeingDebugged;
		BYTE                          Reserved2[1];
		PVOID                         Reserved3[2];
		PPEB_LDR_DATA                 Ldr;
		PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
		BYTE                          Reserved4[104];
		PVOID                         Reserved5[52];
		PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
		BYTE                          Reserved6[128];
		PVOID                         Reserved7[1];
		ULONG                         SessionId;
	} PEB, *PPEB;

	/* hooked function ZwSetKeyValue*/

	typedef NTSTATUS(*ZwSetValueKeyPtr)(
		_In_     HANDLE          KeyHandle,
		_In_     PUNICODE_STRING ValueName,
		_In_opt_ ULONG           TitleIndex,
		_In_     ULONG           Type,
		_In_opt_ PVOID           Data,
		_In_     ULONG           DataSize
		);


	/* Global viariables */

	__declspec(dllimport) KSERVICE_DESCRIPTOR_ENTRY KeServiceDescriptorTable;

	HOOK_INFO HookInfo;

	ZwSetValueKeyPtr oldZwSetValueKey;


	/* Hook operations */

	void DisableReadProtection();
	void EnableReadProtection();


	/* set Write Protector through CR0 */
	void DisableWP_CR0();
	void EnableWP_CR0();


	/* set Write Protector throught (MDL)Memory Descriptor List */
	HOOK_INFO DisableWP_MDL(DWORD32 *ssdt, DWORD32 nServices);
	void EnableWP_MDL(PMDL pMdl, BYTE *callTable);

	/* operations on SSDT */
	DWORD32 GetSSDTIndex(BYTE *addr);
	BYTE *HookSSDT(BYTE *apiCall, BYTE *newAddr, DWORD32 *callTable);
	void UnHookSSDT(BYTE *apiCall, BYTE *oldAddr, DWORD32 *callTable);

	NTSTATUS HookedZwSetValueKey(
		_In_     HANDLE          KeyHandle,
		_In_     PUNICODE_STRING ValueName,
		_In_opt_ ULONG           TitleIndex,
		_In_     ULONG           Type,
		_In_opt_ PVOID           Data,
		_In_     ULONG           DataSize
		);

	
	/* process and register infomation handle */

	NTSTATUS GetRegInfo(HANDLE keyHandle, PUNICODE_STRING regValueName, PROC_RECORD *procRecord);

	NTSTATUS GetProcInfo(HANDLE ProcessId, PROC_RECORD *procRecord);

	VOID ProcNotifyRoutine(
		IN HANDLE  ParentId,
		IN HANDLE  ProcessId,
		IN BOOLEAN  Create
		);


#ifdef __cplusplus
}
#endif

#endif /* _HOOKOPS_H */