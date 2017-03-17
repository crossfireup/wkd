#ifndef _HOOKSSDT_H
#define _HOOKSSDT_H

#include <ntddk.h>
#include <ntstrsafe.h>


#pragma comment(lib, "ntdll")

/* pool tag name */
#define POOLTAG_HOOK_SSDT_01 'Hk01'
#define POOLTAG_HOOK_SSDT_02 'Hk02'

/* device name */
#define NT_DEVICE_NAME      L"\\Device\\SIOCTL"
#define DOS_DEVICE_NAME     L"\\DosDevices\\IoctlTest"

//#pragma warning(disable: 4054 4055)

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

	typedef struct _WP_GLOBALS
	{
		DWORD32 *callTable;	/* address of SSDT mapped to new memory region */
		PMDL pMdl;			/* pointer to MDL */
	}WP_GLOBALS;

	void DisableReadProtection();
	void EnableReadProtection();

	/* set Write Protector through CR0 */
	void DisableWP_CR0();
	void EnableWP_CRO();

	/* set Write Protector throught (MDL)Memory Descriptor List */
	WP_GLOBALS DisableWP_MDL(DWORD32 *ssdt, DWORD32 nServices);
	void EnableWP_MDL(PMDL pMdl, BYTE *callTable);

	/* operations on SSDT */
	DWORD32 GetSSDTIndex(BYTE *addr);
	BYTE *HookSSDT(BYTE *apiCall, BYTE *newAddr, DWORD32 *callTable);
	void UnHookSSDT(BYTE *apiCall, BYTE *oldAddr, DWORD32 *callTable);

	/* hooked function ZwSetKeyValue*/

	typedef NTSTATUS(*ZwSetValueKeyPtr)(
		_In_     HANDLE          KeyHandle,
		_In_     PUNICODE_STRING ValueName,
		_In_opt_ ULONG           TitleIndex,
		_In_     ULONG           Type,
		_In_opt_ PVOID           Data,
		_In_     ULONG           DataSize
		);

	NTSTATUS HookedZwSetValueKey(
		_In_     HANDLE          KeyHandle,
		_In_     PUNICODE_STRING ValueName,
		_In_opt_ ULONG           TitleIndex,
		_In_     ULONG           Type,
		_In_opt_ PVOID           Data,
		_In_     ULONG           DataSize
		);

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
	
	

#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_CRITICAL    1   // Abnormal exit or termination
#define TRACE_LEVEL_FATAL       1   // Deprecated name for Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9

#define TRACE_DRIVER "TraceDriver"

#define DBG 1

#if DBG
#undef TraceEvents

#define TraceEvents(trace_level, trace_event, ...) \
	DbgPrint("HookSSDT.SYS: %s@%s#%d\n", __FILE__, __FUNCTION__, __LINE__); \
	DbgPrint(__VA_ARGS__);

#else
#define HOOKSSDT_KDPRINT(_x_)
#endif


	//
	// WDFDRIVER Events
	//

	DRIVER_INITIALIZE DriverEntry;

	_Dispatch_type_(IRP_MJ_CREATE)
		_Dispatch_type_(IRP_MJ_CLOSE)
		DRIVER_DISPATCH HookSSDTCreateClose;

	_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
		DRIVER_DISPATCH HookSSDTDeviceControl;

	DRIVER_UNLOAD HookSSDTUnload;

	NTSTATUS HookSSDTGetRegInfo(HANDLE keyHandle, UNICODE_STRING regValueName);

	NTSTATUS GetProcInfo(HANDLE ProcessId);

	VOID ProcNotifyRoutine(
		IN HANDLE  ParentId,
		IN HANDLE  ProcessId,
		IN BOOLEAN  Create
		);

#ifdef __cplusplus
}
#endif
#endif /* _HOOKSSDT_H */