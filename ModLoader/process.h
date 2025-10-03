#pragma once

#include "stdafx.h"

class Process
{
public:
    explicit Process(DWORD pid) : m_ProcessId(pid) {}
    ~Process();

    void SetReady();

    template <typename T>
    T ReadValue(intptr_t address)
    {
        char buffer[sizeof(T)];
        size_t nBytesRead;
        bool status = ReadProcessMemory(m_ProcessHandle, (void*)address, &buffer, sizeof(T), &nBytesRead);
        if (!status || nBytesRead != sizeof(T))
            throw std::runtime_error(std::format("Unable to read int16_t at {}", address));
        return *(T*)&buffer;
    }

    int16_t ReadShort(intptr_t address) { return ReadValue<int16_t>(address); }
    int32_t ReadInt32(intptr_t address) { return ReadValue<int32_t>(address); }
    int64_t ReadInt64(intptr_t address) { return ReadValue<int64_t>(address); }
    void ReadString(intptr_t address, char* buffer, size_t n);

    void WriteShort(intptr_t address, int16_t value);
    void WriteInt32(intptr_t address, int32_t value);
    void WriteInt64(intptr_t address, int64_t value);
    void WriteString(intptr_t address, char* buffer);
    void WriteBytes(intptr_t address, unsigned char* bytes, size_t n);

    intptr_t Allocate(size_t size);
    void Free(intptr_t address, size_t size);

    intptr_t GetModuleBaseAddress(const wchar_t* name);
    intptr_t GetProcAddress(intptr_t moduleBaseAddr, const char* name);
    std::wstring GetName();
    HANDLE CreateThread(LPSECURITY_ATTRIBUTES attrs, SIZE_T stackSize, LPTHREAD_START_ROUTINE startAddr, LPVOID param, DWORD flags, DWORD* threadId);

private:
    DWORD m_ProcessId;
    HANDLE m_ProcessHandle = 0;
    std::unordered_map<intptr_t, size_t> m_Allocations{};
};