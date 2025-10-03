#include "stdafx.h"

#include "process.h"

#define MACHINE_64 (WORD)0x8664
#define MACHINE_86 (WORD)0x14C

void Process::SetReady()
{
    m_ProcessHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD, FALSE, m_ProcessId);
    if (m_ProcessHandle == 0)
        throw std::runtime_error(std::format("Unable to open process with id = {}", m_ProcessId));
}

void Process::ReadString(intptr_t address, char* buffer, size_t n) 
{
    size_t nBytesRead;
    bool status = ReadProcessMemory(m_ProcessHandle, (void*)address, buffer, n, &nBytesRead);
    if (!status || nBytesRead != n)
        throw std::runtime_error(std::format("Unable to read string at {}", address));
}

void Process::WriteShort(intptr_t address, int16_t value) 
{
    size_t bytes_written;
    bool status = WriteProcessMemory(m_ProcessHandle, (void*)address, &value, sizeof value, &bytes_written);
    if (!status || bytes_written != sizeof value)
        throw std::runtime_error(std::format("Unable to write int16_t to {}", address));
}

void Process::WriteInt32(intptr_t address, int32_t value) 
{
    size_t bytes_written;
    bool status = WriteProcessMemory(m_ProcessHandle, (void*)address, &value, sizeof value, &bytes_written);
    if (!status || bytes_written != sizeof value)
        throw std::runtime_error(std::format("Unable to write int32_t to {}", address));
}

void Process::WriteInt64(intptr_t address, int64_t value) 
{
    size_t bytes_written;
    bool status = WriteProcessMemory(m_ProcessHandle, (void*)address, &value, sizeof value, &bytes_written);
    if (!status || bytes_written != sizeof value)
        throw std::runtime_error(std::format("Unable to write int32_t to {}", address));
}

void Process::WriteString(intptr_t address, char* buffer) 
{
    size_t buffer_size = strlen(buffer) + 1;
    size_t bytes_written;
    bool status = WriteProcessMemory(m_ProcessHandle, (void*)address, buffer, buffer_size, &bytes_written);
    if (!status || bytes_written != buffer_size)
        throw std::runtime_error(std::format("Unable to write string to {}", address));
}

void Process::WriteBytes(intptr_t address, unsigned char* bytes, size_t n) {
    size_t bytes_written;
    bool status = WriteProcessMemory(m_ProcessHandle, (void*)address, bytes, n, &bytes_written);
    if (!status || bytes_written != n)
        throw std::runtime_error(std::format("Unable to write bytes to {}", address));
}

intptr_t Process::Allocate(size_t size) 
{
    intptr_t address = (intptr_t)VirtualAllocEx(m_ProcessHandle, 0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!address)
        throw std::runtime_error(std::format("Unable to allocate {} bytes", size));
    m_Allocations[address] = size;
    return address;
}

void Process::Free(intptr_t address, size_t size) 
{
    VirtualFreeEx(m_ProcessHandle, (void*)address, size, MEM_DECOMMIT);
}

Process::~Process()
{
    for (const auto& p : m_Allocations)
        Free(p.first, p.second);
    CloseHandle(m_ProcessHandle);
}

intptr_t Process::GetModuleBaseAddress(const wchar_t* name)
{
    DWORD cbNeeded = 0;
    if (!EnumProcessModulesEx(m_ProcessHandle, nullptr, 0, &cbNeeded, LIST_MODULES_ALL)) 
        return 0;

    std::vector<HMODULE> modules;
    modules.resize(cbNeeded / sizeof(HMODULE));

    if (!EnumProcessModulesEx(m_ProcessHandle, modules.data(), (DWORD)(modules.size() * sizeof(HMODULE)), &cbNeeded, LIST_MODULES_ALL))
        return 0;

    wchar_t buffer[MAX_PATH];
    for (size_t i = 0; i < modules.size() && modules[i]; ++i)
    {
        if (GetModuleBaseNameW(m_ProcessHandle, modules[i], buffer, MAX_PATH)) 
        {
            std::wstring curName = buffer;
            for (auto & c : curName) c = towlower(c);
            std::wstring target = name;
            for (auto & c : target) c = towlower(c);

            if (curName == target) 
                return (intptr_t)modules[i];
        }
    }

    return 0;
}

intptr_t Process::GetProcAddress(intptr_t moduleBaseAddr, const char* name)
{
    IMAGE_DOS_HEADER dosHeader = this->ReadValue<IMAGE_DOS_HEADER>(moduleBaseAddr);
    _IMAGE_NT_HEADERS ntHeaders86 = this->ReadValue<_IMAGE_NT_HEADERS>(moduleBaseAddr + dosHeader.e_lfanew);
    WORD machine = ntHeaders86.FileHeader.Machine;

    if (machine != MACHINE_64 && machine != MACHINE_86)
        throw std::runtime_error("Unsupported architecture");

    DWORD dataOffset;
    if (machine == MACHINE_86)
    {
        dataOffset = ntHeaders86.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }
    else
    {
        _IMAGE_NT_HEADERS64 ntHeaders64 = this->ReadValue<_IMAGE_NT_HEADERS64>(moduleBaseAddr + dosHeader.e_lfanew);
        dataOffset = ntHeaders64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    }

    _IMAGE_EXPORT_DIRECTORY dir = this->ReadValue<_IMAGE_EXPORT_DIRECTORY>(moduleBaseAddr + dataOffset);
        
    std::vector<DWORD> nameOffsets{};
    nameOffsets.resize(sizeof(DWORD) * dir.NumberOfNames);

    SIZE_T nBytes;
    ReadProcessMemory(m_ProcessHandle, (void*)(moduleBaseAddr + dir.AddressOfNames), nameOffsets.data(), sizeof(DWORD) * nameOffsets.size(), &nBytes);
    if (nBytes != sizeof(DWORD) * nameOffsets.size())
        throw std::runtime_error("Unable to read names of export functions");

    std::vector<DWORD> funcAddrOffsets{};
    funcAddrOffsets.resize(sizeof(DWORD) * dir.NumberOfFunctions);

    ReadProcessMemory(m_ProcessHandle, (void*)(moduleBaseAddr + dir.AddressOfFunctions), funcAddrOffsets.data(), sizeof(DWORD) * funcAddrOffsets.size(), &nBytes);
    if (nBytes != sizeof(DWORD) * funcAddrOffsets.size())
        throw std::runtime_error("Unable to read function addresses of export functions");

    std::vector<WORD> ordinals{};
    ordinals.resize(sizeof(WORD) * dir.NumberOfNames);

    ReadProcessMemory(m_ProcessHandle, (void*)(moduleBaseAddr + dir.AddressOfNameOrdinals), ordinals.data(), sizeof(WORD) * ordinals.size(), &nBytes);
    if (nBytes != sizeof(WORD) * ordinals.size())
        throw std::runtime_error("Unable to read ordinals of export functions");

    for (size_t i = 0; i < dir.NumberOfNames; ++i)
    {
        char buf[128]{0};
        this->ReadString(moduleBaseAddr + nameOffsets[i], buf, 32);
        if (strcmp(buf, name) == 0)
            return moduleBaseAddr + funcAddrOffsets[ordinals[i]];
    }

    return 0;
}

std::wstring Process::GetName()
{
    HMODULE hMod;
    DWORD cbNeeded;
    wchar_t buffer[MAX_PATH];

    if (EnumProcessModules(m_ProcessHandle, &hMod, sizeof(hMod), &cbNeeded)) 
    {
        if (GetModuleBaseNameW(m_ProcessHandle, hMod, buffer, MAX_PATH)) 
        {
            return buffer;
        }
    }

    return {};
}

HANDLE Process::CreateThread(LPSECURITY_ATTRIBUTES attrs, SIZE_T stackSize, LPTHREAD_START_ROUTINE startAddr, LPVOID param, DWORD flags, DWORD* threadId)
{
    HANDLE thread = CreateRemoteThread(m_ProcessHandle, attrs, stackSize, startAddr, param, flags, threadId);
    if (thread == NULL)
        throw std::runtime_error("Failed to create thread");
    return thread;
}