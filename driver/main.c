#include <ntddk.h>
#include <intrin.h>

#define MSR_CORE_THREAD_COUNT 0x35
#define IA32_THERM_STATUS 0x19c
#define MSR_TEMPERATURE_TARGET 0x1a2

DRIVER_UNLOAD driver_unload;

NTSTATUS driver_entry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	DbgPrint("> [hardware-monitor] driver entry point called\n");

	driver_object->DriverUnload = &driver_unload;
	
	// MSR_CORE_THREAD_COUNT (0x35)
	// [Bits 31:16] Core_COUNT (RO)
	// The number of processor cores that are currently
	// enabled (by either factory configuration or BIOS
	// configuration) in the physical package.
	ULONG core_count = (__readmsr(MSR_CORE_THREAD_COUNT) >> 16) & 0xffff;
	DbgPrint("> [hardware-monitor] core_count: %d\n", core_count);

	// MSR_TEMPERATURE_TARGET (0x1a2)
	// [Bits 23:16] Temperature Target (R)
	// The default thermal throttling or PROCHOT# activation
	// temperature in degrees C. The effective temperature
	// for thermal throttling or PROCHOT# activation is
	// "Temperature Target" + "Target Offset".
	ULONG tj_max = (__readmsr(MSR_TEMPERATURE_TARGET) >> 16) & 0xff;
	DbgPrint("> [hardware-monitor] tj_max: %d °C\n", tj_max);

	for (UINT64 i = 0; i < core_count; i++) {
		KAFFINITY previous_affinity = KeSetSystemAffinityThreadEx(1ULL << i);

		// IA32_THERM_STATUS (0x19c)
		// [Bits 22:16] Digital Readout (RO)
		ULONG value = (__readmsr(IA32_THERM_STATUS) >> 16) & 0x7f;
		ULONG temperature = tj_max - value;
		DbgPrint("> [hardware-monitor] core #%llu: %d °C\n", i + 1, temperature);

		KeRevertToUserAffinityThreadEx(previous_affinity);
	}

	return STATUS_SUCCESS;
}

VOID driver_unload(_In_ PDRIVER_OBJECT driver_object) {
	UNREFERENCED_PARAMETER(driver_object);

	DbgPrint("> [hardware-monitor] driver unloaded\n");
}
