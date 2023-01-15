#include "driver/driver.hpp"

#include <iostream>
#include <Windows.h>

#define MSR_CORE_THREAD_COUNT 0x35
#define IA32_THERM_STATUS 0x19c
#define MSR_TEMPERATURE_TARGET 0x1a2

int main() {
	std::cout << "> client loaded" << std::endl << std::endl;

	std::cout << "> initializing driver" << std::endl << std::endl;
	Driver* driver = new Driver();

	DWORD eax, edx;

	// MSR_CORE_THREAD_COUNT (0x35)
	// [Bits 31:16] Core_COUNT (RO)
	// The number of processor cores that are currently
	// enabled (by either factory configuration or BIOS
	// configuration) in the physical package.
	std::cout << "> reading cpu core count" << std::endl;
	driver->read_msr(MSR_CORE_THREAD_COUNT, &eax, &edx);
	ULONG core_count = (eax >> 16) & 0xffff;
	std::cout << "> core_count: " << core_count << std::endl << std::endl;

	// MSR_TEMPERATURE_TARGET (0x1a2)
	// [Bits 23:16] Temperature Target (R)
	// The default thermal throttling or PROCHOT# activation
	// temperature in degrees C. The effective temperature
	// for thermal throttling or PROCHOT# activation is
	// "Temperature Target" + "Target Offset".
	std::cout << "> reading temperature target" << std::endl;
	driver->read_msr(MSR_TEMPERATURE_TARGET, &eax, &edx);
	ULONG tj_max = (eax >> 16) & 0xff;
	std::cout << "> tj_max: " << tj_max << " \370C" << std::endl << std::endl;

	// IA32_THERM_STATUS (0x19c)
	// [Bits 22:16] Digital Readout (RO)
	std::cout << "> reading temperatures" << std::endl;

	for (std::size_t i = 0; i < core_count; i++) {
		DWORD_PTR mask = 1ULL << i;

		driver->read_msr_affinity(IA32_THERM_STATUS, &eax, &edx, mask);
		ULONG value = (eax >> 16) & 0x7f;
		ULONG temperature = tj_max - value;

		std::cout << "> core #" << i + 1 << ": " << temperature << " \370C" << std::endl;
	}
	std::cout << std::endl;

	std::cout << "> success" << std::endl;

	return TRUE;
}
