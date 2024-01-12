#pragma once

namespace Native {
	class Memory {
		HANDLE hProcess;
		std::unordered_map<intptr_t, size_t> allocations;
	public:
		Memory();
		Memory(HANDLE handle);
		~Memory();
		int16_t ReadShort(intptr_t addr);
		int32_t ReadInt32(intptr_t addr);
		int64_t ReadInt64(intptr_t addr);
		void ReadString(intptr_t addr, char* buffer, size_t len);
		void WriteShort(intptr_t addr, int16_t* value);
		void WriteInt32(intptr_t addr, int32_t* value);
		void WriteInt64(intptr_t addr, int64_t* value);
		void WriteString(intptr_t addr, char* source);
		void WriteBytes(intptr_t addr, char* bytes, size_t bytesLen);
		intptr_t Allocate(size_t size);
		void Free(intptr_t addr, size_t size);
	};
}