#pragma once
#include "Process.h"

#define MACHINE_64 (int16_t)0x8664
#define MACHINE_86 (int16_t)0x14C
#define MACHINE_UNKNOWN 0x0

typedef enum {
    OK = 0,
    NO_PROCESS = 1,
    NO_MONO = 2,
    UNSUPPORTED_MACHINE = 4,
    MONO_EXPORTS_FAIL = 8,
    THREAD_FAIL = 16,
    ACCESS_VIOLATION = 32
} InjectorStatus;

typedef enum {
    NONE = 0,
    GET_ROOT_DOMAIN = 64,
    OPEN_IMAGE_FROM_DATA = 128,
    OPEN_ASSEMBLY_FROM_IMAGE = 256,
    GET_IMAGE_FROM_ASSEMBLY = 512,
    GET_CLASS_FROM_NAME = 1024,
    GET_METHOD_FROM_NAME = 2048,
    GET_CLASS_NAME = 4096,
    RUNTIME_INVOKE = 8192,
    OPEN_ASSEMBLY_FROM = 16384
} ErrorLocation;

namespace Native {
    class MonoProcess : public virtual Process {
        const char* mono_get_root_domain = "mono_get_root_domain";
        const char* mono_thread_attach = "mono_thread_attach";
        const char* mono_image_open_from_data = "mono_image_open_from_data";
        const char* mono_assembly_load_from_full = "mono_assembly_load_from_full";
        const char* mono_assembly_get_image = "mono_assembly_get_image";
        const char* mono_object_get_class = "mono_object_get_class";
        const char* mono_class_from_name = "mono_class_from_name";
        const char* mono_class_get_name = "mono_class_get_name";
        const char* mono_class_get_method_from_name = "mono_class_get_method_from_name";
        const char* mono_runtime_invoke = "mono_runtime_invoke";
        const char* mono_assembly_open = "mono_assembly_open";
        const char* mono_image_get_name = "mono_image_get_name";
        std::unordered_map<const char*, intptr_t> exports = std::unordered_map<const char*, intptr_t>{
            {mono_get_root_domain, 0},
            {mono_thread_attach, 0},
            {mono_image_open_from_data, 0},
            {mono_assembly_load_from_full, 0},
            {mono_assembly_get_image, 0},
            {mono_object_get_class, 0},
            {mono_class_from_name, 0},
            {mono_class_get_name, 0},
            {mono_class_get_method_from_name, 0},
            {mono_runtime_invoke, 0},
            {mono_assembly_open, 0},
            {mono_image_get_name, 0}
        };
        intptr_t monoBaseAddr;
        intptr_t rootDomain;
        bool attach;
        int16_t machine;
    public:
        MonoProcess(DWORD _procId);
        bool GetExports();
        char* Assemble(intptr_t funcPtr, intptr_t retValPtr, intptr_t* params, size_t paramsLen, size_t* resSize);
        char* Assemble86(intptr_t funcPtr, intptr_t retValPtr, intptr_t* params, size_t paramsLen, size_t* resSize);
        char* Assemble64(intptr_t funcPtr, intptr_t retValPtr, intptr_t* params, size_t paramsLen, size_t* resSize);
        intptr_t Execute(intptr_t funcPtr, intptr_t* params, size_t paramsLen, InjectorStatus* status);
        intptr_t GetRootDomain(InjectorStatus* status);
        intptr_t OpenImageFromData(char* assembly, size_t size, InjectorStatus* status);
        intptr_t OpenAssemblyFromImage(intptr_t image, InjectorStatus* status);
        intptr_t GetImageFromAssembly(intptr_t assembly, InjectorStatus* status);
        intptr_t GetClassFromName(intptr_t image, char* _namespace, char* _class, InjectorStatus* status);
        intptr_t GetMethodFromName(intptr_t _class, char* method, InjectorStatus* status);
        bool GetClassName(intptr_t monoObject, char* buffer, InjectorStatus* status);
        bool RuntimeInvoke(intptr_t method, InjectorStatus* status);
        intptr_t OpenAssemblyFrom(char* path, InjectorStatus* status);
        int32_t Inject(char* bytes, size_t bytesLen, char* _namespace, char* _class, char* _method);
        int32_t InjectFrom(char* path, char* _namespace, char* _class, char* _method);
        int32_t LoadDependency(char* bytes, size_t bytesLen);
        int32_t LoadDependencyFrom(char* path);
    private:
        int32_t GetMachine();
        intptr_t GetMonoBaseAddr();
        void Print();
    };
}