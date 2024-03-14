#include "pch.h"
#include "Memory.h"

namespace Native {
	Memory::Memory() : hProcess(NULL) {}

	Memory::Memory(HANDLE handle) : hProcess(handle) {}

	Memory::~Memory() {
		for (const auto& pair : allocations) {
			Free(pair.first, pair.second);
		}
		if (hProcess != NULL)
			CloseHandle(hProcess);
	}

	int16_t Memory::ReadShort(intptr_t addr) {
		int16_t res;
		ReadProcessMemory(hProcess, (LPCVOID)addr, &res, 2, nullptr);
		return res;
	}

	int32_t Memory::ReadInt32(intptr_t addr) {
		int32_t res;
		ReadProcessMemory(hProcess, (LPCVOID)addr, &res, 4, nullptr);
		return res;
	}

	int64_t Memory::ReadInt64(intptr_t addr) {
		int64_t res;
		ReadProcessMemory(hProcess, (LPCVOID)addr, &res, 8, nullptr);
		return res;
	}

	void Memory::ReadString(intptr_t addr, char* buffer, size_t len) {
		ReadProcessMemory(hProcess, (LPCVOID)addr, buffer, len, nullptr);
	}

	void Memory::WriteShort(intptr_t addr, int16_t* value) {
		WriteProcessMemory(hProcess, (LPVOID)addr, value, 2, nullptr);
	}

	void Memory::WriteInt32(intptr_t addr, int32_t* value) {
		WriteProcessMemory(hProcess, (LPVOID)addr, value, 4, nullptr);
	}

	void Memory::WriteInt64(intptr_t addr, int64_t* value) {
		WriteProcessMemory(hProcess, (LPVOID)addr, value, 8, nullptr);
	}

	void Memory::WriteString(intptr_t addr, char* source) {
		size_t len = strlen(source);
		WriteProcessMemory(hProcess, (LPVOID)addr, source, len + 1, nullptr);
	}

	void Memory::WriteBytes(intptr_t addr, char* bytes, size_t bytesLen) {
		WriteProcessMemory(hProcess, (LPVOID)addr, bytes, bytesLen, nullptr);
	}

	intptr_t Memory::Allocate(size_t size) {
		intptr_t alloc = (intptr_t)VirtualAllocEx(hProcess, 0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (alloc != 0)
			allocations[alloc] = size;
		printf("Allocated at %llX %llu bytes\n", alloc, size);
		return alloc;
	}

	void Memory::Free(intptr_t addr, size_t size) {
		bool status = VirtualFreeEx(hProcess, (LPVOID)addr, size, MEM_DECOMMIT);
		printf("Deallocated at %llX %llu bytes\n", addr, size);
	}
}