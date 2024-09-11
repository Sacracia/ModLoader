#pragma once
#include "stdafx.h"
#include "memory.h"

namespace Injector {
	uint32_t GetProcessId(char* process_name);

	class NativeProcess {
	public:
		explicit NativeProcess(void* process_handle, uint32_t process_id) : process_handle_(process_handle), 
			process_id_(process_id), memory_(process_handle) {};

		intptr_t GetModuleBaseAddress(const wchar_t* module_name);
		intptr_t GetProcAddress(intptr_t module_base_address, const char* proc_name);

		Memory memory_;
	private:
		void* process_handle_;
		uint32_t process_id_;
	};
}