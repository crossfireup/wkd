#include "HookEvent.h"
#include "HookSSDT.h"
#include "HookOps.h"

NTSTATUS RegisterEventBasedNotification(
	DEVICE_OBJECT *DeviceObj,
	IRP *Irp)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStackLoc = NULL;
	PDEVICE_EXTENSION pDeviceExt = NULL;
	PNOTIFY_RECORD pNotifyRecord = NULL;
	PREGISTER_EVENT pRegisterEvt = NULL;

	UNREFERENCED_PARAMETER(DeviceObj);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "RegisterEventBasedNotification\n");

	pIoStackLoc = IoGetCurrentIrpStackLocation(Irp);
	pDeviceExt = pIoStackLoc->DeviceObject->DeviceExtension;

	pRegisterEvt = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

	pNotifyRecord = (PNOTIFY_RECORD)ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(REGISTER_EVENT), POOLTAG_HOOK_SSDT_01);

	if (pNotifyRecord == NULL){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ExAllocatePoolWithQuotaTag failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	InitializeListHead(&pNotifyRecord->ListEntry);

	pNotifyRecord->FileObject = pIoStackLoc->FileObject;
	pNotifyRecord->Type = EVENT_BASED;
	pNotifyRecord->DeviceExtension = pIoStackLoc->DeviceObject->DeviceExtension;

	ntStatus = ObReferenceObjectByHandle(pRegisterEvt->hEvent,
		SYNCHRONIZE | EVENT_MODIFY_STATE,
		*ExEventObjectType,
		Irp->RequestorMode,
		&pNotifyRecord->Message.Event,
		NULL
		);

	if (!NT_SUCCESS(ntStatus)){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ObReferenceObjectByHandle failed,errCode:%x\n", ntStatus);
		ExFreePoolWithTag(pNotifyRecord, POOLTAG_HOOK_SSDT_01);
		return ntStatus;
	}

	ExInterlockedInsertTailList(&pDeviceExt->EventQueueHead,
		&pNotifyRecord->ListEntry,
		&pDeviceExt->QueueLock
		);

	return ntStatus;
}

NTSTATUS RegisterIrpBasedNotification(
	DEVICE_OBJECT *DeviceObj,
	IRP *Irp)
{
	PIO_STACK_LOCATION pIoStackLoc = NULL;
	PDEVICE_EXTENSION pDeviceExt = NULL;
	PREGISTER_EVENT pRegisterEvt = NULL;
	PNOTIFY_RECORD pNotifyRecord = NULL;
	KIRQL oldIrql = 0;

	UNREFERENCED_PARAMETER(DeviceObj);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "register Irp-based notification\n");

	pIoStackLoc = IoGetCurrentIrpStackLocation(Irp);
	pDeviceExt = pIoStackLoc->DeviceObject->DeviceExtension;
	pRegisterEvt = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

	pNotifyRecord = ExAllocatePoolWithQuotaTag(NonPagedPool, sizeof(NOTIFY_RECORD), POOLTAG_HOOK_SSDT_01);

	if (pNotifyRecord == NULL){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "ExAllocatePoolWithQuotaTag fialed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	InitializeListHead(&pNotifyRecord->ListEntry);
	pNotifyRecord->FileObject = pIoStackLoc->FileObject;
	pNotifyRecord->DeviceExtension = pDeviceExt;
	pNotifyRecord->Type = IRP_BASED;
	pNotifyRecord->Message.PendingIrp = Irp;

	KeAcquireSpinLock(&pDeviceExt->QueueLock, &oldIrql);

	IoSetCancelRoutine(Irp, HookSSDTCancleRoutine);

	if (Irp->Cancel){
		if (IoSetCancelRoutine(Irp, NULL) != NULL){
			KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrql);

			ExFreePoolWithTag(pNotifyRecord, POOLTAG_HOOK_SSDT_01);

			return STATUS_CANCELLED;
		}
	}

	IoMarkIrpPending(Irp);
	InsertTailList(&pDeviceExt->EventQueueHead, &pNotifyRecord->ListEntry);

	pNotifyRecord->CancelRoutineFreeMemory = FALSE;

	Irp->Tail.Overlay.DriverContext[3] = pNotifyRecord;

	KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrql);

	return STATUS_PENDING;
}

NTSTATUS SetNotifyEvent(PUNICODE_STRING deviceName, HANDLE Pid, HANDLE keyHandle, PUNICODE_STRING regValueName)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObj = NULL;
	PFILE_OBJECT pFileObj = NULL;
	PDEVICE_EXTENSION pDeviceExt = NULL;
	PNOTIFY_RECORD pNotifyRecord = { 0 };
	KIRQL oldIrql = 0;
	PLIST_ENTRY firstEntry = NULL;

	PAGED_CODE();

	ntStatus = IoGetDeviceObjectPointer(deviceName, 
		FILE_READ_ACCESS | FILE_WRITE_ACCESS, 
		&pFileObj, 
		&pDeviceObj);

	if (!NT_SUCCESS(ntStatus)){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "IoGetDeviceObjectPointer failed, errCode: %x\n", ntStatus);
		return ntStatus;
	}

	pDeviceExt = (PDEVICE_EXTENSION)pDeviceObj->DeviceExtension;

	ASSERT(pDeviceExt != NULL);

	KeAcquireSpinLock(&pDeviceExt->QueueLock, &oldIrql);

	if (IsListEmpty(&pDeviceExt->EventQueueHead) == TRUE){
		ntStatus = STATUS_UNSUCCESSFUL;
		KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrql);
		goto err;
	}

	firstEntry = RemoveHeadList(&pDeviceExt->EventQueueHead);

	KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrql);

	pNotifyRecord = (PNOTIFY_RECORD)CONTAINING_RECORD(firstEntry, NOTIFY_RECORD, ListEntry);

	ntStatus = IoAcquireRemoveLock(pNotifyRecord->FileObject->FsContext, Pid);
	if (!NT_SUCCESS(ntStatus)){
		KeResetEvent(pNotifyRecord->Message.Event);
		goto err;
	}

	pNotifyRecord->ProcRecord.Pid = Pid;
	ntStatus = GetProcInfo(Pid, &pNotifyRecord->ProcRecord);
	if (!NT_SUCCESS(ntStatus)){
		goto err;
	}

	ntStatus = GetRegInfo(keyHandle, regValueName, &pNotifyRecord->ProcRecord);


err:
	ObDereferenceObject(pDeviceObj);
	ObDereferenceObject(pFileObj);	

	if (pNotifyRecord != NULL){
		if (!NT_SUCCESS(ntStatus)){
			KeResetEvent(pNotifyRecord->Message.Event);

			if (pNotifyRecord->ProcRecord.ImagePath.Buffer != NULL)
				RtlFreeUnicodeString(&pNotifyRecord->ProcRecord.ImagePath);

			if (pNotifyRecord->ProcRecord.RegPath.Buffer != NULL)
				RtlFreeUnicodeString(&pNotifyRecord->ProcRecord.RegPath);
		}
		else{
			KeSetEvent(pNotifyRecord->Message.Event, 0, FALSE);
		}
	}

	return ntStatus;
}