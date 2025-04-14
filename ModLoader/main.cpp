#include "stdafx.h"
#include "injector.h"

#include <fstream>

static bool IsUnityProcess(DWORD processId, bool includeIL2CPP) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    MODULEENTRY32 me{};
    me.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(snapshot, &me)) {
        do {
            if (wcsncmp(me.szModule, L"mono", 4) == 0 || includeIL2CPP && wcscmp(me.szModule, L"GameAssembly.dll") == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &me));
    }
    CloseHandle(snapshot);
    return false;
}

struct Process {
    std::wstring processName;
    DWORD processId;
};

DWORD ChooseProcess(bool includeIL2CPP) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(PROCESSENTRY32);

    std::vector<Process> processes{};

    if (Process32First(snapshot, &pe)) {
        do {
            if (IsUnityProcess(pe.th32ProcessID, includeIL2CPP))
                processes.emplace_back(pe.szExeFile, pe.th32ProcessID);
        } while (Process32Next(snapshot, &pe));
    }
    CloseHandle(snapshot);

    std::string input;
    if (processes.size() == 1)
        return processes[0].processId;
    for (Process p : processes) {
        while (true) {
            std::wcout << p.processName << " ? (Y/n): ";
            std::cin >> input;
            if (input[0] == 'n')
                break;
            if (input[0] == 'Y')
                return p.processId;
        }
    }

    return 0;
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

int main(int argc, char* argv[])
{
    try {
        const char* config_path = "config.cfg";
        if (!std::filesystem::exists(config_path))
            throw std::runtime_error("Config doesnt exist\n");

        DWORD processId = 0;

        std::ifstream config(config_path);
        std::string line;
        while (std::getline(config, line)) {
            auto argv = Split(line);
            if (argv[0] == "mono-dep") {
                if (!processId && (processId = ChooseProcess(false)) == 0)
                    throw std::runtime_error("No game to load into");
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading dependency " << argv[1].data() << ": ";
                Injector::InjectAssembly(processId, full_dll_path.data());
                std::cout << "OK\n";
            }
            if (argv[0] == "mono-ex") {
                if (!processId && (processId = ChooseProcess(false)) == 0)
                    throw std::runtime_error("No game to load into");
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading mod " << argv[1].data() << ": ";
                Injector::InjectAssemblyEx(processId, full_dll_path.data(), argv[2].data(), argv[3].data(), argv[4].data());
                std::cout << "OK\n";
            }
            if (argv[0] == "native") {
                if (!processId && (processId = ChooseProcess(true)) == 0)
                    throw std::runtime_error("No game to load into");
                std::string full_dll_path = std::filesystem::absolute(argv[1]).string();
                std::cout << "Loading dll " << argv[1].data() << ": ";
                Injector::InjectDll(processId, full_dll_path.data());
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