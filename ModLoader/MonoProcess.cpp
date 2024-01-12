#include "pch.h"
#include "MonoProcess.h"
#include "Assembler.h"

namespace Native {
	MonoProcess::MonoProcess(DWORD _procId) : Process(_procId) {
		attach = false;
		rootDomain = NULL;
		machine = GetMachine();
		monoBaseAddr = GetMonoBaseAddr();
	}

	int32_t MonoProcess::GetMachine() {
		if (hProcess == NULL)
			return MACHINE_UNKNOWN;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
		MODULEENTRY32 me;
		me.dwSize = sizeof(MODULEENTRY32);
		Module32First(snapshot, &me);
		CloseHandle(snapshot);
		intptr_t procBaseAddr = (intptr_t)me.modBaseAddr;
		int32_t e_lfanew = mem->ReadInt32(procBaseAddr + 0x3C);
		int16_t _machine = mem->ReadShort(procBaseAddr + e_lfanew + 0x4);
		if (_machine == MACHINE_64)
			return MACHINE_64;
		if (_machine == MACHINE_86)
			return MACHINE_86;
		return MACHINE_UNKNOWN;
	}

	intptr_t MonoProcess::GetMonoBaseAddr() {
		if (hProcess == NULL || machine == MACHINE_UNKNOWN)
			return NULL;
		MODULEENTRY32 me;
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
		me.dwSize = sizeof(MODULEENTRY32);
		Module32First(snapshot, &me);
		do {
			if (wcsncmp(me.szModule, L"mono", 4) == 0)
				return (intptr_t)me.modBaseAddr;
		} while (Module32Next(snapshot, &me));
		return NULL;
	}

	bool MonoProcess::GetExports() {
		if (monoBaseAddr == NULL)
			return false;
		int32_t e_lfanew = mem->ReadInt32(monoBaseAddr + 0x3C);
		intptr_t ntHeaders = monoBaseAddr + e_lfanew;
		intptr_t optionalHeader = ntHeaders + 0x18;
		intptr_t dataDirectory = optionalHeader + (machine == MACHINE_64 ? 0x70 : 0x60);
		intptr_t exportDirectory = monoBaseAddr + mem->ReadInt32(dataDirectory);
		int32_t count = mem->ReadInt32(exportDirectory + 0x18);
		intptr_t funcs = monoBaseAddr + mem->ReadInt32(exportDirectory + 0x1C);
		intptr_t names = monoBaseAddr + mem->ReadInt32(exportDirectory + 0x20);
		intptr_t ordinals = monoBaseAddr + mem->ReadInt32(exportDirectory + 0x24);
		intptr_t addr;
		int32_t offset;
		int16_t ordinal;
		char buffer[128];
		for (int32_t i = 0; i < count; ++i) {
			offset = mem->ReadInt32(names + i * 4);
			mem->ReadString(monoBaseAddr + offset, buffer, 32);
			ordinal = mem->ReadShort(ordinals + i * 2);
			addr = monoBaseAddr + mem->ReadInt32(funcs + ordinal * 4);
			if (addr == 0)
				continue;
			for (auto& pair : exports) {
				if (strcmp(pair.first, buffer) == 0)
					pair.second = addr;
			}
		}
		Print();
		for (const auto& pair : exports)
			if (pair.second == 0)
				return false;
		return true;
	}

	char* MonoProcess::Assemble86(intptr_t functionPtr, intptr_t retValPtr, intptr_t* params, size_t size, size_t* resSize) {
		Assembler asmr = Assembler();
		if (attach) {
			asmr.Push(rootDomain);
			asmr.MovToEax(exports[mono_thread_attach]);
			asmr.CallEax();
			asmr.AddEsp(4);
		}
		for (int i = size - 1; i >= 0; i--)
			asmr.Push(params[i]);

		asmr.MovToEax(functionPtr);
		asmr.CallEax();
		asmr.AddEsp((unsigned char)size * 4);
		asmr.MovEaxTo(retValPtr);
		asmr.Return();
		return asmr.ToByteArray(resSize);
	}

	char* MonoProcess::Assemble64(intptr_t functionPtr, intptr_t retValPtr, intptr_t* params, size_t size, size_t* resSize) {
		Assembler asmr = Assembler();
		asmr.SubRsp(40);
		if (attach) {
			asmr.MovToRax(exports[mono_thread_attach]);
			asmr.MovToRcx(rootDomain);
			asmr.CallRax();
		}
		asmr.MovToRax(functionPtr);
		for (int i = 0; i < size; ++i) {
			if (i == 0)
				asmr.MovToRcx(params[i]);
			if (i == 1)
				asmr.MovToRdx(params[i]);
			if (i == 2)
				asmr.MovToR8(params[i]);
			if (i == 3)
				asmr.MovToR9(params[i]);
		}
		asmr.CallRax();
		asmr.AddRsp(40);
		asmr.MovRaxTo(retValPtr);
		asmr.Return();
		return asmr.ToByteArray(resSize);
	}

	char* MonoProcess::Assemble(intptr_t functionPtr, intptr_t retValPtr, intptr_t* params, size_t size, size_t* resSize) {
		return machine == MACHINE_64 ? Assemble64(functionPtr, retValPtr, params, size, resSize)
			: Assemble86(functionPtr, retValPtr, params, size, resSize);
	}

	intptr_t MonoProcess::Execute(intptr_t functionPtr, intptr_t* params, size_t size, InjectorStatus* status) {
		intptr_t retValPtr = mem->Allocate(machine == MACHINE_64 ? 8 : 4);
		size_t resSize;
		char* code = Assemble(functionPtr, retValPtr, params, size, &resSize);
		intptr_t alloc = mem->Allocate(resSize);
		mem->WriteBytes(alloc, code, resSize);
		delete[] code;
		HANDLE thread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)alloc, NULL, NULL, NULL);
		if (thread == NULL) {
			*status = THREAD_FAIL;
			return 0;
		}
		DWORD result = WaitForSingleObject(thread, -1);
		if (result != 0) {
			*status = THREAD_FAIL;
			return 0;
		}
		intptr_t ret = machine == MACHINE_64 ? (intptr_t)mem->ReadInt64(retValPtr) : (intptr_t)mem->ReadInt32(retValPtr);
		if ((int64_t)ret == 0x00000000C0000005) {
			*status = ACCESS_VIOLATION;
			return 0;
		}
		*status = OK;
		return ret;
	}

	intptr_t MonoProcess::GetRootDomain(InjectorStatus* status)
	{
		intptr_t rootDomain = Execute(exports[mono_get_root_domain], nullptr, 0, status);
		return rootDomain;
	}

	intptr_t MonoProcess::OpenImageFromData(char* assembly, size_t size, InjectorStatus* status) {
		intptr_t monoImageOpenStatus = mem->Allocate(4);
		intptr_t alloc = mem->Allocate(size);
		mem->WriteBytes(alloc, assembly, size);
		intptr_t params[4] = { alloc, (intptr_t)size, 1, monoImageOpenStatus };
		intptr_t rawImage = Execute(exports[mono_image_open_from_data], params, 4, status);
		if (*status != OK)
			return NULL;
		int32_t openStatus = mem->ReadInt32(monoImageOpenStatus);
		if (openStatus != 0)
			return NULL;
		return rawImage;
	}

	intptr_t MonoProcess::OpenAssemblyFromImage(intptr_t image, InjectorStatus* status) {
		intptr_t monoImageOpenStatus = mem->Allocate(4);
		intptr_t fname = mem->Allocate(1);
		intptr_t params[4] = { image, fname, monoImageOpenStatus, 0 };
		intptr_t assembly = Execute(exports[mono_assembly_load_from_full], params, 4, status);
		if (*status != OK)
			return NULL;
		int32_t openStatus = mem->ReadInt32(monoImageOpenStatus);
		if (openStatus != 0)
			return NULL;
		return assembly;
	}

	intptr_t MonoProcess::GetImageFromAssembly(intptr_t assembly, InjectorStatus* status) {
		intptr_t params[1] = { assembly };
		intptr_t image = Execute(exports[mono_assembly_get_image], params, 1, status);
		return image;
	}

	intptr_t MonoProcess::GetClassFromName(intptr_t image, char* _namespace, char* _class, InjectorStatus* status) {
		intptr_t _nstring = mem->Allocate(strlen(_namespace) + 1);
		mem->WriteString(_nstring, _namespace);
		intptr_t _cstring = mem->Allocate(strlen(_class) + 1);
		mem->WriteString(_cstring, _class);
		intptr_t params[3] = { image, _nstring, _cstring };
		intptr_t classPtr = Execute(exports[mono_class_from_name], params, 3, status);
		return classPtr;
	}

	intptr_t MonoProcess::GetMethodFromName(intptr_t _class, char* method, InjectorStatus* status) {
		intptr_t methodNamePtr = mem->Allocate(strlen(method) + 1);
		mem->WriteString(methodNamePtr, method);
		intptr_t params[3] = { _class, methodNamePtr, 0 };
		intptr_t methodPtr = Execute(exports[mono_class_get_method_from_name], params, 3, status);
		return methodPtr;
	}

	bool MonoProcess::GetClassName(intptr_t monoObject, char* buffer, InjectorStatus* status) {
		intptr_t params[1] = { monoObject };
		intptr_t classPtr = Execute(exports[mono_object_get_class], params, 1, status);
		if (classPtr == NULL)
			return false;
		intptr_t params2[1] = { classPtr };
		intptr_t classNamePtr = Execute(exports[mono_class_get_name], params2, 1, status);
		if (classNamePtr == NULL)
			return false;
		mem->WriteString(classNamePtr, buffer);
		return true;
	}

	bool MonoProcess::RuntimeInvoke(intptr_t method, InjectorStatus* status) {
		intptr_t excPtr = mem->Allocate(machine == MACHINE_64 ? 8 : 4);
		intptr_t params[4] = { method, 0, 0, excPtr };
		intptr_t result = Execute(exports[mono_runtime_invoke], params, 4, status);
		intptr_t exc = (intptr_t)mem->ReadInt32(excPtr);
		if (exc != 0)
			return false;
		return true;
	}

	intptr_t MonoProcess::OpenAssemblyFrom(char* path, InjectorStatus* status) {
		intptr_t monoImageOpenStatus = mem->Allocate(4);
		intptr_t pathPtr = mem->Allocate(strlen(path) + 1);
		mem->WriteString(pathPtr, path);
		intptr_t params[2] = { pathPtr, monoImageOpenStatus };
		intptr_t result = Execute(exports[mono_assembly_open], params, 2, status);
		return result;
	}

	void MonoProcess::Print() {
		for (const auto& pair : exports) {
			printf("%s = %llX\n", pair.first, pair.second);
		}
	}

	int32_t MonoProcess::Inject(char* bytes, size_t bytesLen, char* _namespace, char* _class, char* _method) {
		try {
			if (hProcess == NULL)
				return NO_PROCESS;
			if (monoBaseAddr == NULL)
				return NO_MONO;
			if (machine == MACHINE_UNKNOWN)
				return UNSUPPORTED_MACHINE;
			if (!GetExports())
				return MONO_EXPORTS_FAIL;
			InjectorStatus status;
			if (rootDomain == NULL) {
				rootDomain = GetRootDomain(&status);
				if (rootDomain == NULL)
					return GET_ROOT_DOMAIN | status;
			}
			intptr_t rawImage = OpenImageFromData(bytes, bytesLen, &status);
			if (rawImage == NULL)
				return OPEN_IMAGE_FROM_DATA | status;
			attach = true;
			intptr_t assembly = OpenAssemblyFromImage(rawImage, &status);
			if (assembly == NULL)
				return OPEN_ASSEMBLY_FROM_IMAGE | status;
			intptr_t image = GetImageFromAssembly(assembly, &status);
			if (image == NULL)
				return GET_IMAGE_FROM_ASSEMBLY | status;
			intptr_t classPtr = GetClassFromName(image, _namespace, _class, &status);
			if (classPtr == NULL)
				return GET_CLASS_FROM_NAME | status;
			intptr_t methodPtr = GetMethodFromName(classPtr, _method, &status);
			if (methodPtr == NULL)
				return GET_METHOD_FROM_NAME | status;
			bool flag = RuntimeInvoke(methodPtr, &status);
			if (!flag)
				return RUNTIME_INVOKE | status;
			attach = false;
			return 0;
		}
		catch (...) {
			return -1;
		}
	}

	int32_t MonoProcess::InjectFrom(char* path, char* _namespace, char* _class, char* _method) {
		try {
			printf("path = %s\n", path);
			printf("_namespace = %s\n", _namespace);
			printf("_class = %s\n", _class);
			printf("_method = %s\n", _method);
			if (hProcess == NULL)
				return NO_PROCESS;
			if (monoBaseAddr == NULL)
				return NO_MONO;
			if (machine == MACHINE_UNKNOWN)
				return UNSUPPORTED_MACHINE;
			if (!GetExports())
				return MONO_EXPORTS_FAIL;
			InjectorStatus status;
			if (rootDomain == NULL) {
				rootDomain = GetRootDomain(&status);
				if (rootDomain == NULL)
					return GET_ROOT_DOMAIN | status;
			}
			attach = true;
			intptr_t assembly = OpenAssemblyFrom(path, &status);
			if (assembly == NULL)
				return OPEN_ASSEMBLY_FROM | status;
			intptr_t image = GetImageFromAssembly(assembly, &status);
			if (image == NULL)
				return GET_IMAGE_FROM_ASSEMBLY | status;
			intptr_t classPtr = GetClassFromName(image, _namespace, _class, &status);
			if (classPtr == NULL)
				return GET_CLASS_FROM_NAME | status;
			intptr_t methodPtr = GetMethodFromName(classPtr, _method, &status);
			if (methodPtr == NULL)
				return GET_METHOD_FROM_NAME | status;
			bool flag = RuntimeInvoke(methodPtr, &status);
			if (!flag)
				return RUNTIME_INVOKE | status;
			attach = false;
			return 0;
		}
		catch (...) {
			return -1;
		}
	}

	int32_t MonoProcess::LoadDependency(char* bytes, size_t bytesLen) {
		try {
			if (hProcess == NULL)
				return NO_PROCESS;
			if (monoBaseAddr == NULL)
				return NO_MONO;
			if (machine == MACHINE_UNKNOWN)
				return UNSUPPORTED_MACHINE;
			if (!GetExports())
				return MONO_EXPORTS_FAIL;
			InjectorStatus status;
			if (rootDomain == NULL) {
				rootDomain = GetRootDomain(&status);
				if (rootDomain == NULL)
					return GET_ROOT_DOMAIN | status;
			}
			intptr_t rawImage = OpenImageFromData(bytes, bytesLen, &status);
			if (rawImage == NULL)
				return OPEN_IMAGE_FROM_DATA | status;
			attach = true;
			intptr_t assembly = OpenAssemblyFromImage(rawImage, &status);
			if (assembly == NULL)
				return OPEN_ASSEMBLY_FROM_IMAGE | status;
			attach = false;
			return 0;
		}
		catch (...) {
			return -1;
		}
	}

	int32_t MonoProcess::LoadDependencyFrom(char* path) {
		try {
			printf("path = %s\n", path);
			if (hProcess == NULL)
				return NO_PROCESS;
			if (monoBaseAddr == NULL)
				return NO_MONO;
			if (machine == MACHINE_UNKNOWN)
				return UNSUPPORTED_MACHINE;
			if (!GetExports())
				return MONO_EXPORTS_FAIL;
			InjectorStatus status;
			if (rootDomain == NULL) {
				rootDomain = GetRootDomain(&status);
				printf("rootdomain = %llX\n", rootDomain);
				if (rootDomain == NULL)
					return GET_ROOT_DOMAIN | status;
			}
			attach = true;
			intptr_t assembly = OpenAssemblyFrom(path, &status);
			printf("assembly = %llX\n", assembly);
			if (assembly == NULL)
				return OPEN_ASSEMBLY_FROM | status;
			attach = false;
			return 0;
		}
		catch (...) {
			return -1;
		}
	}
}