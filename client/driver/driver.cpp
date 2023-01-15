#include "driver.hpp"

#include <stdexcept>
#include <string>

Driver::Driver() {
	handle = CreateFile(L"\\\\.\\hardware-monitor", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Driver::Driver: failed to retrieve device handle");
	}
}

Driver::~Driver() {
	if (handle) {
		CloseHandle(handle);
	}
}

void WINAPI Driver::read_msr(DWORD msr_index, PDWORD eax, PDWORD edx) {
	if (handle == NULL || eax == NULL || edx == NULL) {
		throw std::invalid_argument("Driver::read_msr: one or more arguments were NULL");
	}

	BYTE output_buffer[8];
	memset(output_buffer, 0, sizeof(output_buffer));

	BOOL result = DeviceIoControl(handle, IOCTL_READ_MSR, &msr_index, sizeof(msr_index), &output_buffer, sizeof(output_buffer), NULL, NULL);

	if (result == FALSE) {
		throw std::runtime_error("Driver::read_msr: IOCTL call with index " + std::to_string(msr_index) + " failed");
	}
	
	memcpy(eax, output_buffer, 4);
	memcpy(edx, output_buffer + 4, 4);
}

void WINAPI Driver::read_msr_affinity(DWORD msr_index, PDWORD eax, PDWORD edx, DWORD_PTR mask) {
	if (handle == NULL || eax == NULL || edx == NULL) {
		throw std::invalid_argument("Driver::read_msr: one or more arguments were NULL");
	}

	HANDLE thread = GetCurrentThread();
	DWORD_PTR previous_mask = SetThreadAffinityMask(thread, mask);

	read_msr(msr_index, eax, edx);

	SetThreadAffinityMask(thread, previous_mask);
}
