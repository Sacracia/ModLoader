namespace Injector {
    void InjectDll(DWORD process_id, char* dll_path);
    void InjectAssembly(DWORD process_id, char* dll_path);
    void InjectAssemblyEx(DWORD process_id, char* dll_path, char* name_space, char* class_name, char* method_name);
    //void EjectAssemblyEx(char* process_name, char* assembly_name, char* name_space, char* class_name, char* method_name);
}