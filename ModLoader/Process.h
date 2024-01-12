#pragma once
#include "Memory.h"

namespace Native {
	class Process {
	public:
		HANDLE hProcess;
		Memory* mem;
		DWORD procId;
		bool handleClosed;
		Process(DWORD _procId);
		~Process();
		intptr_t GetModuleBaseAddress(wchar_t* moduleName);
	};
}