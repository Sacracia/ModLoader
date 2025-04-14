#include "stdafx.h"
#include "injector.h"

#include <fstream>

#include "native_process.h"
#include "mono_process.h"

namespace Injector {
    void InjectDll(DWORD process_id, char* dll_path) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        void* process_handle = OpenProcess(PROCESS_ALL_ACCESS, true, process_id);
        NativeProcess native_process(process_handle, process_id);

        intptr_t address = native_process.memory_.Allocate(strlen(dll_path) + 1);
        native_process.memory_.WriteString(address, (char*)dll_path);

        intptr_t kernel_base_address = native_process.GetModuleBaseAddress(L"KERNEL32.DLL");
        intptr_t proc_address = native_process.GetProcAddress(kernel_base_address, "LoadLibraryA");

        void* thread = CreateRemoteThread(process_handle, NULL, 0, (LPTHREAD_START_ROUTINE)proc_address, (void*)address, 0, NULL);
        if (thread == NULL)
            throw std::runtime_error("Failed to create thread");

        DWORD exitCode;
        WaitForSingleObject(thread, INFINITE);
        GetExitCodeThread(thread, &exitCode);
    }

    void InjectAssembly(DWORD process_id, char* dll_path) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        void* process_handle = OpenProcess(PROCESS_ALL_ACCESS, true, process_id);
        MonoProcess mono_process(process_handle, process_id);
        mono_process.Init();

        intptr_t assembly = mono_process.OpenAssemblyFrom(true, dll_path);
    }

    void InjectAssemblyEx(DWORD process_id, char* dll_path, char* name_space, char* class_name, char* method_name) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        void* process_handle = OpenProcess(PROCESS_ALL_ACCESS, true, process_id);
        MonoProcess mono_process(process_handle, process_id);
        mono_process.Init();

        intptr_t assembly_address = mono_process.OpenAssemblyFrom(true, dll_path);
        intptr_t image_address = mono_process.GetImageFromAssembly(true, assembly_address);
        intptr_t class_address = mono_process.GetClassFromName(true, image_address, name_space, class_name);
        intptr_t method_address = mono_process.GetMethodFromName(true, class_address, method_name);
        mono_process.RuntimeInvoke(true, method_address);
    }

    /*void EjectAssemblyEx(char* process_name, char* assembly_name, char* name_space, char* class_name, char* method_name) {
        uint32_t process_id = GetProcessId(process_name);
        void* process_handle = OpenProcess(PROCESS_ALL_ACCESS, true, process_id);
        MonoProcess mono_process(process_handle, process_id);
        mono_process.Init();

        intptr_t image_address = mono_process.GetImageLoaded(true, assembly_name);
        if (image_address == 0)
            return;
        intptr_t assembly_address = mono_process.GetAssemblyFromImage(true, image_address);
        intptr_t class_address = mono_process.GetClassFromName(true, image_address, name_space, class_name);
        intptr_t method_address = mono_process.GetMethodFromName(true, class_address, method_name);
        mono_process.RuntimeInvoke(true, method_address);

        mono_process.CloseAssembly(true, assembly_address);
        mono_process.CloseImage(true, image_address);
    }*/
}