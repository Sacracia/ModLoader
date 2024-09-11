#pragma once
#include "stdafx.h"

namespace Injector {
	class Memory {
	public:
		explicit Memory(void* process_handle) : process_handle_(process_handle) {};
		~Memory();

		int16_t ReadShort(intptr_t address);
		int32_t ReadInt32(intptr_t address);
		int64_t ReadInt64(intptr_t address);
		void ReadString(intptr_t address, char* buffer, size_t n);

		void WriteShort(intptr_t address, int16_t value);
		void WriteInt32(intptr_t address, int32_t value);
		void WriteInt64(intptr_t address, int64_t value);
		void WriteString(intptr_t address, char* buffer);
		void WriteBytes(intptr_t address, unsigned char* bytes, size_t n);

		intptr_t Allocate(size_t size);
		void Free(intptr_t address, size_t size);
	private:
		void* process_handle_;
		std::unordered_map<intptr_t, size_t> allocations_;
	};
}