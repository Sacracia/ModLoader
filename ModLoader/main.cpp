#include "stdafx.h"

#include "process.h"

static std::wstring GetProcessName(DWORD pid)
{
    HMODULE hMod;
    DWORD cbNeeded;
    wchar_t buffer[MAX_PATH];

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) 
    {
        if (GetModuleBaseNameW(hProcess, hMod, buffer, MAX_PATH)) 
        {
            return buffer;
        }
    }

    return {};
}

static bool LoadModIntoProcess(DWORD pid, const char* path)
{
    size_t nBytesNeeded = strlen(path) + 1;
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_CREATE_THREAD, FALSE, pid);
    LPVOID address = VirtualAllocEx(hProcess, 0, nBytesNeeded, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!address)
    {
        std::cout << "Unable to allocate address\n";
        return false;
    }
    
    SIZE_T nBytes;
    BOOL status = WriteProcessMemory(hProcess, address, path, nBytesNeeded, &nBytes);
    if (status == 0 || nBytesNeeded != nBytes)
    {
        std::cout << "Unable to write the whole path\n";
        return false;
    }


}

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cout << "Usage: Drag and drop file\n";
            system("pause");
            return 0;
        }

        std::filesystem::path dllPath{ argv[1] };

        if (!std::filesystem::exists(dllPath))
        {
            std::cout << "Non-existing file passed!\n";
            system("pause");
            return 0;
        }

        if (dllPath.extension() != ".dll")
        {
            std::cout << "Invalid file passed!\n";
            system("pause");
            return 0;
        }

        static std::vector<HWND> s_UnityWindows{};

        WNDENUMPROC enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL
            {
                wchar_t buff[64]{ 0 };
                GetClassNameW(hwnd, buff, sizeof(buff) / sizeof(wchar_t));

                if (wcsncmp(buff, L"UnityWndClass", 13) == 0)
                    s_UnityWindows.push_back(hwnd);

                return TRUE;
            };

        EnumWindows(enumProc, 0);

        DWORD pid = 0;
        std::cout << "Unity games:\n";
        for (HWND hwnd : s_UnityWindows)
        {
            if (GetWindowThreadProcessId(hwnd, &pid) != 0)
            {
                Process process{ pid };
                process.SetReady();
                std::wcout << process.GetName() << '\n';
            }
        }

        if (pid > 0)
        {
            Process process{ pid };
            process.SetReady();

            std::wcout << "Loading mod into " << process.GetName() << " ...\n";

            size_t pathLen = strlen(argv[1]) + 1;
            intptr_t pathAddr = process.Allocate(pathLen);
            process.WriteString(pathAddr, argv[1]);

            intptr_t proc = process.GetProcAddress(process.GetModuleBaseAddress(L"KERNEL32.DLL"), "LoadLibraryA");
            HANDLE thread = process.CreateThread(0, 0, (LPTHREAD_START_ROUTINE)proc, (void*)pathAddr, 0, nullptr);
            DWORD exitCode;
            WaitForSingleObject(thread, INFINITE);
            if (GetExitCodeThread(thread, &exitCode) == 0)
                throw std::runtime_error("Get exit code thread error");

            std::cout << "Exit code thread = " << std::hex << exitCode << '\n';
        }
    }
    catch (const std::runtime_error& err)
    {
        std::cout << err.what() << '\n';
    }

    system("pause");
    return 0;
}