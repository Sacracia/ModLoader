#include "pch.h"
#include "Process.h"

namespace Native {
	Process::Process(DWORD _procId) {
		procId = _procId;
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, true, _procId);
		mem = new Memory(hProcess);
		if (hProcess == NULL)
			handleClosed = true;
	}

	Process::~Process() {
		delete mem;
	}

	intptr_t Process::GetModuleBaseAddress(wchar_t* moduleName) {
		if (handleClosed)
			return NULL;
		MODULEENTRY32 me;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
		me.dwSize = sizeof(MODULEENTRY32);
		Module32First(snapshot, &me);
		do {
			if (wcscmp(me.szModule, moduleName))
				return (intptr_t)me.modBaseAddr;
		} while (Module32Next(snapshot, &me));
		return NULL;
	}
}