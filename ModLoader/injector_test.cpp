#include "stdafx.h"
#include "native_process.h"
#include "mono_process.h"

#include <fstream>

namespace Injector {
    void InjectDll(char* process_name, char* dll_path) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        uint32_t process_id = GetProcessId(process_name);
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

    void InjectAssembly(char* process_name, char* dll_path) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        uint32_t process_id = GetProcessId(process_name);
        void* process_handle = OpenProcess(PROCESS_ALL_ACCESS, true, process_id);
        MonoProcess mono_process(process_handle, process_id);
        mono_process.Init();

        intptr_t assembly = mono_process.OpenAssemblyFrom(true, dll_path);
    }

    void InjectAssemblyEx(char* process_name, char* dll_path, char* name_space, char* class_name, char* method_name) {
        if (!std::filesystem::exists(dll_path))
            throw std::runtime_error("File doesn\'t exist");

        uint32_t process_id = GetProcessId(process_name);
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

std::vector<std::string> Split(std::string& s) {
    std::vector<std::string> argv;
    int left = -1;
    int right = 0;
    while (left != s.size()) {
        while (right < s.size() && s[right] != ' ')
            ++right;
        argv.push_back(std::string{ s.begin() + 1 + left, s.begin() + right });
        left = right++;
    }
    return argv;
}

int main()
{
    try {
        const char* config_path = "config.cfg";
        if (!std::filesystem::exists(config_path))
            throw std::runtime_error("No config specified\n");

        std::ifstream config(config_path);
        std::string game;
        std::getline(config, game);

        std::string line;
        while (std::getline(config, line)) {
            auto argv = Split(line);
            if (argv[0] == "mono-dep") {
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading dependency " << argv[1].data() << ": ";
                Injector::InjectAssembly(game.data(), full_dll_path.data());
                std::cout << "OK\n";
            }
            if (argv[0] == "mono-ex") {
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading mod " << argv[1].data() << ": ";
                Injector::InjectAssemblyEx(game.data(), full_dll_path.data(), argv[2].data(), argv[3].data(), argv[4].data());
                std::cout << "OK\n";
            }
            /*if (argv[0] == "mono-ej") {
                std::cout << "Unloading mod " << argv[1].data() << ": ";
                Injector::EjectAssemblyEx(game.data(), argv[1].data(), argv[2].data(), argv[3].data(), argv[4].data());
                std::cout << "OK\n";
            }*/
        }
        std::cout << "All modules loaded\n";
    }
    catch (const std::runtime_error& error) {
        std::cout << error.what() << "\n";
    }
    catch (const std::exception& ex) {
        std::cout << ex.what() << "\n";
    }
    system("pause");
    return 0;
}