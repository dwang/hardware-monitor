#include <ntddk.h>
#include <intrin.h>

#define NT_DEVICE_NAME	L"\\Device\\hardware-monitor"
#define DOS_DEVICE_NAME	L"\\DosDevices\\hardware-monitor"

#define IOCTL_READ_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MSR_CORE_THREAD_COUNT 0x35
#define IA32_THERM_STATUS 0x19c
#define MSR_TEMPERATURE_TARGET 0x1a2

DRIVER_INITIALIZE driver_entry;
_Dispatch_type_(IRP_MJ_CREATE) _Dispatch_type_(IRP_MJ_CLOSE) DRIVER_DISPATCH driver_create_close;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH driver_device_control;
DRIVER_UNLOAD driver_unload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, driver_entry)
#pragma alloc_text(PAGE, driver_create_close)
#pragma alloc_text(PAGE, driver_device_control)
#pragma alloc_text(PAGE, driver_unload)
#endif

NTSTATUS driver_entry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) {
	NTSTATUS status;
	PDEVICE_OBJECT device_object = NULL;
	UNICODE_STRING nt_device_name;
	UNICODE_STRING win32_device_name;

	UNREFERENCED_PARAMETER(registry_path);

	DbgPrint("> [hardware-monitor] driver entry point called\n");

	RtlInitUnicodeString(&nt_device_name, NT_DEVICE_NAME);

	status = IoCreateDevice(driver_object, 0, &nt_device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);
	
	if (!NT_SUCCESS(status)) {
		DbgPrint("> [hardware-monitor] IoCreateDevice failed\n");
		return status;
	}

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver_create_close;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver_create_close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_device_control;
	driver_object->DriverUnload = driver_unload;

	RtlInitUnicodeString(&win32_device_name, DOS_DEVICE_NAME);

	status = IoCreateSymbolicLink(&win32_device_name, &nt_device_name);

	if (!NT_SUCCESS(status)) {
		DbgPrint("> [hardware-monitor] IoCreateSymbolicLink failed\n");
		IoDeleteDevice(device_object);
	}
	
	DbgPrint("> [hardware-monitor] driver initialized\n");

	return status;
}

NTSTATUS driver_create_close(PDEVICE_OBJECT device_object, PIRP irp) {
	UNREFERENCED_PARAMETER(device_object);

	PAGED_CODE();

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS driver_device_control(PDEVICE_OBJECT device_object, PIRP irp) {
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	PCHAR input_buffer, output_buffer;
	PIO_STACK_LOCATION irp_stack;
	ULONG msr_index;
	ULONGLONG msr_data;
	
	UNREFERENCED_PARAMETER(device_object);

	irp_stack = IoGetCurrentIrpStackLocation(irp);
	
	if (irp_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_READ_MSR) {
		input_buffer = irp->AssociatedIrp.SystemBuffer;
		output_buffer = irp->AssociatedIrp.SystemBuffer;

		try {
			msr_index = *((ULONG*)input_buffer);

			if (msr_index == MSR_CORE_THREAD_COUNT || msr_index == IA32_THERM_STATUS || msr_index == MSR_TEMPERATURE_TARGET) {
				msr_data = __readmsr(msr_index);
				RtlCopyMemory(output_buffer, &msr_data, 8);
				irp->IoStatus.Information = 8;
				status = STATUS_SUCCESS;
			}
		} except(EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
			DbgPrint("> [hardware-monitor] IOCTL_READ_MSR failed\n");
		}
	}

	irp->IoStatus.Status = status;

	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}


VOID driver_unload(_In_ PDRIVER_OBJECT driver_object) {
	PDEVICE_OBJECT device_object = driver_object->DeviceObject;
	UNICODE_STRING win32_device_name;

	PAGED_CODE();

	RtlInitUnicodeString(&win32_device_name, DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&win32_device_name);

	if (device_object != NULL) {
		IoDeleteDevice(device_object);
	}

	DbgPrint("> [hardware-monitor] driver unloaded\n");
}
