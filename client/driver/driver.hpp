#pragma once

#include <Windows.h>
#include <winioctl.h>

#define IOCTL_READ_MSR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

class Driver {
private:
	HANDLE handle;

public:
	Driver();
	~Driver();

	void WINAPI read_msr(DWORD msr_index, PDWORD eax, PDWORD edx);
	void WINAPI read_msr_affinity(DWORD msr_index, PDWORD eax, PDWORD edx, DWORD_PTR mask);
};
