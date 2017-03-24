#include "HookEvent.h"
#include "HookSSDT.h"

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