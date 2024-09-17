namespace Injector {
    void InjectDll(char* process_name, char* dll_path);
    void InjectAssembly(char* process_name, char* dll_path);
    void InjectAssemblyEx(char* process_name, char* dll_path, char* name_space, char* class_name, char* method_name);
    //void EjectAssemblyEx(char* process_name, char* assembly_name, char* name_space, char* class_name, char* method_name);
}