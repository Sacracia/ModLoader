#include "stdafx.h"
#include "native_process.h"

#define MACHINE_64 (int16_t)0x8664
#define MACHINE_86 (int16_t)0x14C

namespace Injector {
	uint32_t GetProcessId(char* process_name) {
		size_t length = strlen(process_name);
		std::unique_ptr<wchar_t[]> wide_name(new wchar_t[length + 1]);
		size_t out_size;
		mbstowcs_s(&out_size, wide_name.get(), length + 1, process_name, length);

		void* snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		uint32_t pid = 0;

		PROCESSENTRY32 process_entry;
		process_entry.dwSize = sizeof PROCESSENTRY32;
		Process32First(snapshot_handle, &process_entry);

		do {
			if (wcscmp(wide_name.get(), process_entry.szExeFile) == 0) {
				pid = process_entry.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot_handle, &process_entry));

		CloseHandle(snapshot_handle);

		#ifdef DEVELOP_MODE
		printf("process pid %u\n", pid);
		#endif
		if (pid == 0)
			throw std::runtime_error("Process not found");
		return pid;
	}

	intptr_t NativeProcess::GetModuleBaseAddress(const wchar_t* module_name) {
		void* snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);
		intptr_t address = 0;

		MODULEENTRY32 module_entry;
		module_entry.dwSize = sizeof MODULEENTRY32;
		Module32First(snapshot_handle, &module_entry);

		do {
			if (wcscmp(module_entry.szModule, module_name) == 0) {
				address = (intptr_t)module_entry.modBaseAddr;
				break;
			}
		} while (Module32Next(snapshot_handle, &module_entry));
		CloseHandle(snapshot_handle);
		return address;
	}

	intptr_t NativeProcess::GetProcAddress(intptr_t module_base_address, const char* proc_name) {
		int32_t e_lfanew = memory_.ReadInt32(module_base_address + 0x3C);
		int16_t machine = memory_.ReadShort(module_base_address + e_lfanew + 0x4);
		if (machine != MACHINE_64 && machine != MACHINE_86)
			throw std::runtime_error("Unsupported architecture");
		intptr_t ntHeaders = module_base_address + e_lfanew;
		intptr_t optionalHeader = ntHeaders + 0x18;
		intptr_t dataDirectory = optionalHeader + (machine == MACHINE_64 ? 0x70 : 0x60);
		intptr_t exportDirectory = module_base_address + memory_.ReadInt32(dataDirectory);

		int32_t count = memory_.ReadInt32(exportDirectory + 0x18);
		intptr_t funcs = module_base_address + memory_.ReadInt32(exportDirectory + 0x1C);
		intptr_t names = module_base_address + memory_.ReadInt32(exportDirectory + 0x20);
		intptr_t ordinals = module_base_address + memory_.ReadInt32(exportDirectory + 0x24);

		intptr_t address;
		int32_t offset;
		int16_t ordinal;
		char buffer[128];
		for (int32_t i = 0; i < count; ++i) {
			offset = memory_.ReadInt32(names + i * 4);
			memory_.ReadString(module_base_address + offset, buffer, 32);
			ordinal = memory_.ReadShort(ordinals + i * 2);
			address = module_base_address + memory_.ReadInt32(funcs + ordinal * 4);
			if (address == 0)
				continue;
			if (strcmp(buffer, proc_name) == 0)
				return address;
		}
		return 0;
	}
}