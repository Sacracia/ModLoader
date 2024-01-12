#include "pch.h"
#include "MonoProcessWrapper.h"

int ModLoader::MonoProcess::LoadMod(array<System::Byte>^ assemblyBytes, int bytesLen, String^ _namespace, String^ _class, String^ _method) {
    pin_ptr<System::Byte> p = &assemblyBytes[0];
    unsigned char* pby = p;
    char* pch = reinterpret_cast<char*>(pby);
    char* nspace = (char*)(Marshal::StringToHGlobalAnsi(_namespace)).ToPointer();
    char* cl = (char*)(Marshal::StringToHGlobalAnsi(_namespace)).ToPointer();
    char* meth = (char*)(Marshal::StringToHGlobalAnsi(_namespace)).ToPointer();
    return mp->Inject(pch, bytesLen, nspace, cl, meth);
}

int ModLoader::MonoProcess::LoadModFrom(String^ path, String^ _namespace, String^ _class, String^ _method) {
    char* _path = (char*)(Marshal::StringToHGlobalAnsi(path)).ToPointer();
    char* nspace = (char*)(Marshal::StringToHGlobalAnsi(_namespace)).ToPointer();
    char* cl = (char*)(Marshal::StringToHGlobalAnsi(_class)).ToPointer();
    char* meth = (char*)(Marshal::StringToHGlobalAnsi(_method)).ToPointer();
    return mp->InjectFrom(_path, nspace, cl, meth);
}

int ModLoader::MonoProcess::LoadDependency(array<System::Byte>^ assemblyBytes, int bytesLen) {
    pin_ptr<System::Byte> p = &assemblyBytes[0];
    unsigned char* pby = p;
    char* pch = reinterpret_cast<char*>(pby);
    return mp->LoadDependency(pch, bytesLen);
}

int ModLoader::MonoProcess::LoadDependencyFrom(String^ path) {
    char* _path = (char*)(Marshal::StringToHGlobalAnsi(path)).ToPointer();
    return mp->LoadDependencyFrom(_path);
}