#ifndef _HOOKSSDT_H
#define _HOOKSSDT_H

#include <ntddk.h>
#include <ntstrsafe.h>

#include "public.h"
#include "HookEvent.h"

/* type cast from function ptr to Byte *  warning */
#pragma warning(disable: 4054 4055)

#ifdef __cplusplus
extern "C" {
#endif


/* pool tag name */
#define POOLTAG_HOOK_SSDT_01 'Hk01'
#define POOLTAG_HOOK_SSDT_02 'Hk02'

/* device name */
#define NT_DEVICE_NAME      L"\\Device\\SysMon"
#define DOS_DEVICE_NAME     L"\\DosDevices\\SysMon"

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

	typedef struct _PROC_RECORD {
		HANDLE Pid;
		UNICODE_STRING RegPath;
		UNICODE_STRING ImagePath;
	} PROC_RECORD;

	typedef struct _DEVICE_EXTENSION {
		PDEVICE_OBJECT  Self;
		LIST_ENTRY      EventQueueHead; // where all the user notification requests are queued
		KSPIN_LOCK      QueueLock;
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

	typedef struct _NOTIFY_RECORD{
		NOTIFY_TYPE     Type;
		LIST_ENTRY      ListEntry;
		union {
			PKEVENT     Event;
			PIRP        PendingIrp;
		} Message;
		KDPC            Dpc;
		KTIMER          Timer;
		PFILE_OBJECT    FileObject;
		PDEVICE_EXTENSION   DeviceExtension;
		BOOLEAN         CancelRoutineFreeMemory;
		PROC_RECORD		ProcRecord;
	} NOTIFY_RECORD, *PNOTIFY_RECORD;

	typedef struct _FILE_CONTEXT{
		//
		// Lock to rundown threads that are dispatching I/Os on a file handle 
		// while the cleanup for that handle is in progress.
		//
		IO_REMOVE_LOCK  FileRundownLock;
	} FILE_CONTEXT, *PFILE_CONTEXT;

	//
	// WDFDRIVER Events
	//

	DRIVER_INITIALIZE DriverEntry;

	_Dispatch_type_(IRP_MJ_CREATE)
		_Dispatch_type_(IRP_MJ_CLOSE)
		DRIVER_DISPATCH HookSSDTCreateClose;
	
	_Dispatch_type_(IRP_MJ_CLEANUP)
	DRIVER_DISPATCH HookSSDTCleanup;

	_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
		DRIVER_DISPATCH HookSSDTDeviceControl;

	DRIVER_UNLOAD HookSSDTUnload;

	DRIVER_CANCEL HookSSDTCancleRoutine;

#ifdef __cplusplus
}
#endif

#endif /* _HOOKSSDT_H */