#include <iostream>
#include <Windows.h>
#include <winioctl.h>

#define IOCTL_READ_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MSR_CORE_THREAD_COUNT 0x35
#define IA32_THERM_STATUS 0x19c
#define MSR_TEMPERATURE_TARGET 0x1a2

BOOL WINAPI read_msr(HANDLE driver_handle, DWORD msr_index, PDWORD eax, PDWORD edx) {
	if (eax == NULL || edx == NULL) {
		return FALSE;
	}

	BYTE output_buffer[8];
	memset(output_buffer, 0, sizeof(output_buffer));

	BOOL result = DeviceIoControl(driver_handle, IOCTL_READ_MSR, &msr_index, sizeof(msr_index), &output_buffer, sizeof(output_buffer), NULL, NULL);

	if (result) {
		memcpy(eax, output_buffer, 4);
		memcpy(edx, output_buffer + 4, 4);
	}

	return result;
}

int main() {
	std::cout << "> client loaded" << std::endl << std::endl;

	std::cout << "> initializing driver" << std::endl << std::endl;

	HANDLE driver_handle = CreateFile(L"\\\\.\\hardware-monitor", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (driver_handle == INVALID_HANDLE_VALUE) {
		std::cout << "> failed to initialize driver" << std::endl;
		return STATUS_INVALID_HANDLE;
	}

	DWORD eax, edx;
	BOOL result;

	// MSR_CORE_THREAD_COUNT (0x35)
	// [Bits 31:16] Core_COUNT (RO)
	// The number of processor cores that are currently
	// enabled (by either factory configuration or BIOS
	// configuration) in the physical package.
	std::cout << "> reading cpu core count" << std::endl;

	result = read_msr(driver_handle, MSR_CORE_THREAD_COUNT, &eax, &edx);

	if (result == FALSE) {
		std::cout << "> failed to read MSR_CORE_THREAD_COUNT" << std::endl << std::endl;
		return result;
	}

	ULONG core_count = (eax >> 16) & 0xffff;
	std::cout << "> core_count: " << core_count << std::endl << std::endl;

	// MSR_TEMPERATURE_TARGET (0x1a2)
	// [Bits 23:16] Temperature Target (R)
	// The default thermal throttling or PROCHOT# activation
	// temperature in degrees C. The effective temperature
	// for thermal throttling or PROCHOT# activation is
	// "Temperature Target" + "Target Offset".
	std::cout << "> reading temperature target" << std::endl;

	result = read_msr(driver_handle, MSR_TEMPERATURE_TARGET, &eax, &edx);

	if (result == FALSE) {
		std::cout << "> failed to read MSR_TEMPERATURE_TARGET" << std::endl << std::endl;
		return result;
	}

	ULONG tj_max = (eax >> 16) & 0xff;
	std::cout << "> tj_max: " << tj_max << " \370C" << std::endl << std::endl;

	// IA32_THERM_STATUS (0x19c)
	// [Bits 22:16] Digital Readout (RO)
	std::cout << "> reading temperatures" << std::endl;

	DWORD_PTR mask, previous_mask;
	HANDLE thread;

	for (std::size_t i = 0; i < core_count; i++) {
		mask = 1ULL << i;
		thread = GetCurrentThread();
		previous_mask = SetThreadAffinityMask(thread, mask);

		result = read_msr(driver_handle, IA32_THERM_STATUS, &eax, &edx);

		SetThreadAffinityMask(thread, previous_mask);

		if (result == FALSE) {
			std::cout << "> failed to read IA32_THERM_STATUS" << std::endl << std::endl;
			return result;
		}

		ULONG value = (eax >> 16) & 0x7f;
		ULONG temperature = tj_max - value;

		std::cout << "> core #" << i + 1 << ": " << temperature << " \370C" << std::endl;
	}
	std::cout << std::endl;

	CloseHandle(driver_handle);

	std::cout << "> success" << std::endl;

	return TRUE;
}
