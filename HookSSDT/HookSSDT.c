#include "HookSSDT.h"
#include "HookOps.h"


NTSTATUS DriverEntry(
	_In_ struct _DRIVER_OBJECT *DriverObject,
	_In_ PUNICODE_STRING       RegistryPath
	)
{
	NTSTATUS ntStatus;
	UNICODE_STRING uniNtDevName = { 0 };
	UNICODE_STRING uniDosDevName = { 0 };
	PDEVICE_OBJECT deviceObj = NULL;
	PDEVICE_EXTENSION pDeviceExt = NULL;

	UNREFERENCED_PARAMETER(RegistryPath);

	RtlInitUnicodeString(&uniNtDevName, NT_DEVICE_NAME);

	ntStatus = IoCreateDevice(DriverObject,
		sizeof(DEVICE_EXTENSION),
		&uniNtDevName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&deviceObj
		);

	if (!NT_SUCCESS(ntStatus)){
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "IoCreateDevice failed \n");
		return(ntStatus);
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = HookSSDTCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = HookSSDTCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = HookSSDTCleanup;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HookSSDTDeviceControl;
	DriverObject->DriverUnload = HookSSDTUnload;

	RtlInitUnicodeString(&uniDosDevName, DOS_DEVICE_NAME);
	ntStatus = IoCreateSymbolicLink(&uniDosDevName, &uniNtDevName);

	if (!NT_SUCCESS(ntStatus)){
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "IoCreateSymbolicLink failed \n");
		IoDeleteDevice(deviceObj);
		return(ntStatus);
	}


	HookInfo = DisableWP_MDL(KeServiceDescriptorTable.KiServiceTable, KeServiceDescriptorTable.nSyscalls);
	if (HookInfo.pMdl == NULL || HookInfo.callTable == NULL){
		return STATUS_UNSUCCESSFUL;
	}

	oldZwSetValueKey = (ZwSetValueKeyPtr)HookSSDT((BYTE *)ZwSetValueKey, (BYTE *)HookedZwSetValueKey, HookInfo.callTable);

	/* init process create and exit notify routine */
	ntStatus = PsSetCreateProcessNotifyRoutine(ProcNotifyRoutine, FALSE);
	if (!NT_SUCCESS(ntStatus)){
		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "failed, errCode=%d\n", ntStatus);
	}

	/* Initialize the device extension */
	pDeviceExt = deviceObj->DeviceExtension;

	InitializeListHead(&pDeviceExt->EventQueueHead);

	KeInitializeSpinLock(&pDeviceExt->QueueLock);

	pDeviceExt->Self = deviceObj;

	/* establish user-buffer access method */
	deviceObj->Flags = DO_BUFFERED_IO;

	return(ntStatus);
}

NTSTATUS HookSSDTCreateClose(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
	)
{
	PIO_STACK_LOCATION pIoStackLoc = NULL;
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	PFILE_CONTEXT pFileCtx = NULL;

	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	pIoStackLoc = IoGetCurrentIrpStackLocation(Irp);

	ASSERT(pIoStackLoc->FileObject != NULL);

	switch (pIoStackLoc->MajorFunction){
	case IRP_MJ_CREATE:
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "IRQ_MJ_CREATE\n");

		pFileCtx = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_CONTEXT), POOLTAG_HOOK_SSDT_01);

		if (NULL == pFileCtx){
			ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "call ExAllocatePoolWithTag failed, errorCode=%x\n", ntStatus);
			break;
		}

		IoInitializeRemoveLock(&pFileCtx->FileRundownLock, POOLTAG_HOOK_SSDT_01, 0, 0);

		/* make sure FsContext not being used*/
		ASSERT(pIoStackLoc->FileObject->FsContext == NULL);

		pIoStackLoc->FileObject->FsContext = (PVOID)pFileCtx;

		ntStatus = STATUS_SUCCESS;
		break;

	case IRP_MJ_CLOSE:
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "IRQ_MJ_CLOSE\n");

		pFileCtx = pIoStackLoc->FileObject->FsContext;

		ExFreePoolWithTag(pFileCtx, POOLTAG_HOOK_SSDT_01);

		ntStatus = STATUS_SUCCESS;
		break;

	default:
		ASSERT(FALSE);
		ntStatus = STATUS_NOT_IMPLEMENTED;
		break;
	}

	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return ntStatus;
}

NTSTATUS HookSSDTCleanup(
	_Inout_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP           *Irp
	)
{
	NTSTATUS ntStatus;
	PIO_STACK_LOCATION pIoStackLoc = NULL;
	PDEVICE_EXTENSION pDeviceExt = NULL;
	PFILE_CONTEXT pFileCtx = NULL;
	KIRQL oldIrq = 0;
	PLIST_ENTRY pListHead = NULL, curEntry = NULL, nextEntry = NULL;
	PNOTIFY_RECORD pNotifyRecord = NULL;
	LIST_ENTRY cleanupList = { 0 };

	pIoStackLoc = IoGetCurrentIrpStackLocation(Irp);
	pDeviceExt = DeviceObject->DeviceExtension;

	ASSERT(pIoStackLoc->FileObject != NULL);
	pFileCtx = pIoStackLoc->FileObject->FsContext;

	/* acquire cannot fial for that you cannot cleanup a handler for more than one time */
	ntStatus = IoAcquireRemoveLock(&pFileCtx->FileRundownLock, Irp);
	ASSERT(NT_SUCCESS(ntStatus));
	
	/* Wait for all the threads that currently dispatching exit and prevent any threads dispatch
	* I/O on the same handle 
	*/
	IoReleaseRemoveLockAndWait(&pFileCtx->FileRundownLock, Irp);

	InitializeListHead(&cleanupList);

	/* remove all the pending notification records that belong to the file handle */
	KeAcquireSpinLock(&pDeviceExt->QueueLock, &oldIrq);

	pListHead = &pDeviceExt->EventQueueHead;

	for (curEntry = pListHead->Flink; curEntry != pListHead; curEntry = nextEntry){
		pNotifyRecord = CONTAINING_RECORD(curEntry, NOTIFY_RECORD, ListEntry);

		nextEntry = curEntry->Flink;
		if (pIoStackLoc->FileObject == pNotifyRecord->FileObject){
			if (!KeCancelTimer(&pNotifyRecord->Timer)){
				TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "cancled timer");
				RemoveEntryList(curEntry);

				switch (pNotifyRecord->Type){
				case IRP_BASED:
					/* Clear the cancel - routine and check the return value to
					 * see whether it was cleared by us or by the I / O manager.
					 */
					if (IoSetCancelRoutine(pNotifyRecord->Message.PendingIrp, NULL) != NULL){
						/* We cleared it and as a result we own the IRP and
						 * nobody can cancel it anymore.We will queue the IRP
						 * in the local cleanup list so that we can complete
						 * all the IRPs outside the lock to avoid deadlocks in
						 * the completion routine of the driver above us re - enters
						 * our driver.
						 */
						InsertTailList(&cleanupList, &pNotifyRecord->Message.PendingIrp->Tail.Overlay.ListEntry);
						ExFreePoolWithTag(pNotifyRecord, POOLTAG_HOOK_SSDT_01);
					}
					else {
						/* The I / O manager cleared it and called the cancel - routine.
						 * Cancel routine is probably waiting to acquire the lock.
						 * So reinitialze the ListEntry so that it doesn't crash
						 * when it tries to remove the entry from the list and
						 * set the CancelRoutineFreeMemory to indicate that it should
						 * free the notification record.
						 */
						InitializeListHead(&pNotifyRecord->ListEntry);
						pNotifyRecord->CancelRoutineFreeMemory = TRUE;
					}
					break;
				case EVENT_BASED:
					ObDereferenceObject(pNotifyRecord->Message.Event);
					ExFreePoolWithTag(pNotifyRecord, POOLTAG_HOOK_SSDT_01);
					break;

				}
			}
		}
	}

	KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrq);

	while (!IsListEmpty(&cleanupList)){
		PIRP pPendingIrp = NULL;

		curEntry = RemoveHeadList(&cleanupList);
		pPendingIrp = CONTAINING_RECORD(curEntry, IRP, Tail.Overlay.ListEntry);

		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "cancled IRP: %p\n", pPendingIrp);

		pPendingIrp->Tail.Overlay.DriverContext[3] = NULL;
		pPendingIrp->IoStatus.Information = 0;
		pPendingIrp->IoStatus.Status = STATUS_CANCELLED;

		IoCompleteRequest(pPendingIrp, IO_NO_INCREMENT);
	}

	/* complet the cleanup IRP*/
	Irp->IoStatus.Information = 0; 
	Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Cleanup finished");

	return ntStatus;
}


NTSTATUS HookSSDTDeviceControl(
	_Inout_  struct _DEVICE_OBJECT *DeviceObject,
	_Inout_  struct _IRP *Irp
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStackLoc = NULL;
	PREGISTER_EVENT pRegisterEvt = NULL;
	PFILE_CONTEXT pFileCtx = NULL;

	UNREFERENCED_PARAMETER(DeviceObject);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "start IO control\n");

	pIoStackLoc = IoGetCurrentIrpStackLocation(Irp);

	ASSERT(pIoStackLoc->FileObject != NULL);

	pFileCtx = pIoStackLoc->FileObject->FsContext;

	ntStatus = IoAcquireRemoveLock(&pFileCtx->FileRundownLock, Irp);

	if (!NT_SUCCESS(ntStatus)){
		Irp->IoStatus.Status = ntStatus;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		
		return(ntStatus);
	}

	switch (pIoStackLoc->Parameters.DeviceIoControl.IoControlCode){
	case IOCTL_REGISTER_EVENT:
		TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "IOCTL_REGISTER_EVENT\n");

		if (pIoStackLoc->Parameters.DeviceIoControl.InputBufferLength < sizeof(REGISTER_EVENT)){
			TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "REGISTER_EVENT size too small\n");
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
		}

		pRegisterEvt = (PREGISTER_EVENT)Irp->AssociatedIrp.SystemBuffer;

		switch (pRegisterEvt->Type){
		case IRP_BASED:
			ntStatus = RegisterIrpBasedNotification(DeviceObject, Irp);
			break;

		case EVENT_BASED:
			ntStatus = RegisterEventBasedNotification(DeviceObject, Irp);
			break;

		default:
			ASSERTMSG("\t Unknown notification type from user mode\n", FALSE);
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
		}
	default:
		ASSERT(FALSE);  // should never hit this
		ntStatus = STATUS_NOT_IMPLEMENTED;
		break;
	}

	if (ntStatus != STATUS_PENDING){
		Irp->IoStatus.Status = ntStatus;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	IoReleaseRemoveLock(&pFileCtx->FileRundownLock, Irp);

	return ntStatus;
}


VOID HookSSDTCancleRoutine(
	DEVICE_OBJECT *DeviceObject,
	IRP *Irp)
{
	PDEVICE_EXTENSION pDeviceExt = NULL;
	PNOTIFY_RECORD pNotifyRecord = NULL;
	KIRQL oldIrql = 0;

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "cancling routine irp: %p\n", Irp);

	pDeviceExt = DeviceObject->DeviceExtension;

	IoReleaseCancelSpinLock(Irp->CancelIrql);

	KeAcquireSpinLock(&pDeviceExt->QueueLock, &oldIrql);

	pNotifyRecord = Irp->Tail.Overlay.DriverContext[3];
	ASSERT(pNotifyRecord != NULL);

	ASSERT(pNotifyRecord->Type == IRP_BASED);

	RemoveEntryList(&pNotifyRecord->ListEntry);
	pNotifyRecord->Message.PendingIrp = NULL;

	if (pNotifyRecord->CancelRoutineFreeMemory == FALSE) {
		//
		// This is case 1 where the DPC is waiting to run.
		//
		InitializeListHead(&pNotifyRecord->ListEntry);
	}
	else {
		//
		// This is either 2 or 3.
		//
		ExFreePoolWithTag(pNotifyRecord, POOLTAG_HOOK_SSDT_01);
		pNotifyRecord = NULL;
	}

	KeReleaseSpinLock(&pDeviceExt->QueueLock, oldIrql);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "cancled routine irp: %p\n", Irp);

	Irp->Tail.Overlay.DriverContext[3] = NULL;
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_CANCELLED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "cancle routine irp: %p\n", Irp);
	
	return;
}

VOID HookSSDTUnload(
	_In_ struct _DRIVER_OBJECT *DriverObject
	)
{
	PDEVICE_OBJECT deviceObj = DriverObject->DeviceObject;
	PDEVICE_EXTENSION pDeviceExt = deviceObj->DeviceExtension;
	UNICODE_STRING uniDosDevName = { 0 };

	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "Unload driver\n");

	if (!IsListEmpty(&pDeviceExt->EventQueueHead)){
		ASSERTMSG("Event Queue is not empty\n", FALSE);
	}

	UnHookSSDT((BYTE *)ZwSetValueKey, (BYTE *)oldZwSetValueKey, HookInfo.callTable);

	EnableWP_MDL(HookInfo.pMdl, (BYTE *)HookInfo.callTable);

	PsSetCreateProcessNotifyRoutine(ProcNotifyRoutine, TRUE);

	/* delete user-mode symbolic link and device object */
	RtlInitUnicodeString(&uniDosDevName, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&uniDosDevName);

	if (deviceObj != NULL){
		IoDeleteDevice(deviceObj);
	}

	return;
}

