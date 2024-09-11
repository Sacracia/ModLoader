#include "stdafx.h"
#include "memory.h"

namespace Injector {
	int16_t Memory::ReadShort(intptr_t address) {
		int16_t value;
		size_t bytes_read;
		bool status = ReadProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_read);
		if (!status || bytes_read != sizeof value)
			throw std::runtime_error(std::format("Unable to read int16_t at {}", address));
		return value;
	}

	int32_t Memory::ReadInt32(intptr_t address) {
		int32_t value;
		size_t bytes_read;
		bool status = ReadProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_read);
		if (!status || bytes_read != sizeof value)
			throw std::runtime_error(std::format("Unable to read int32_t at {}", address));
		return value;
	}

	int64_t Memory::ReadInt64(intptr_t address) {
		int64_t value;
		size_t bytes_read;
		bool status = ReadProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_read);
		if (!status || bytes_read != sizeof value)
			throw std::runtime_error(std::format("Unable to read int64_t at {}", address));
		return value;
	}

	void Memory::ReadString(intptr_t address, char* buffer, size_t n) {
		size_t bytes_read;
		bool status = ReadProcessMemory(process_handle_, (void*)address, buffer, n, &bytes_read);
		if (!status || bytes_read != n)
			throw std::runtime_error(std::format("Unable to read string at {}", address));
	}

	void Memory::WriteShort(intptr_t address, int16_t value) {
		size_t bytes_written;
		bool status = WriteProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_written);
		if (!status || bytes_written != sizeof value)
			throw std::runtime_error(std::format("Unable to write int16_t to {}", address));
	}

	void Memory::WriteInt32(intptr_t address, int32_t value) {
		size_t bytes_written;
		bool status = WriteProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_written);
		if (!status || bytes_written != sizeof value)
			throw std::runtime_error(std::format("Unable to write int32_t to {}", address));
	}

	void Memory::WriteInt64(intptr_t address, int64_t value) {
		size_t bytes_written;
		bool status = WriteProcessMemory(process_handle_, (void*)address, &value, sizeof value, &bytes_written);
		if (!status || bytes_written != sizeof value)
			throw std::runtime_error(std::format("Unable to write int32_t to {}", address));
	}

	void Memory::WriteString(intptr_t address, char* buffer) {
		size_t buffer_size = strlen(buffer) + 1;
		size_t bytes_written;
		bool status = WriteProcessMemory(process_handle_, (void*)address, buffer, buffer_size, &bytes_written);
		if (!status || bytes_written != buffer_size)
			throw std::runtime_error(std::format("Unable to write string to {}", address));
	}

	void Memory::WriteBytes(intptr_t address, unsigned char* bytes, size_t n) {
		size_t bytes_written;
		bool status = WriteProcessMemory(process_handle_, (void*)address, bytes, n, &bytes_written);
		if (!status || bytes_written != n)
			throw std::runtime_error(std::format("Unable to write bytes to {}", address));
	}

	intptr_t Memory::Allocate(size_t size) {
		intptr_t address = (intptr_t)VirtualAllocEx(process_handle_, 0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (address == NULL)
			throw std::runtime_error(std::format("Unable to allocate {} bytes", size));
		allocations_[address] = size;
		#ifdef DEVELOP_MODE
		std::cout << std::format("{} bytes allocated at {}\n", size, address);
		#endif
		return address;
	}

	void Memory::Free(intptr_t address, size_t size) {
		VirtualFreeEx(process_handle_, (void*)address, size, MEM_DECOMMIT);
		#ifdef DEVELOP_MODE
		std::cout << std::format("{} bytes free at {}\n", size, address);
		#endif
	}

	Memory::~Memory() {
		for (const auto& p : allocations_)
			Free(p.first, p.second);
	}
}