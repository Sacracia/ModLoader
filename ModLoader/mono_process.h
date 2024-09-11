#pragma once
#include "stdafx.h"
#include "memory.h"

#define MACHINE_64 (int16_t)0x8664
#define MACHINE_86 (int16_t)0x14C

namespace Injector {
	namespace {
		struct CachedData {
			intptr_t mono_base_address;
			int16_t machine;
			intptr_t root_domain;
			std::unordered_map<const char*, intptr_t> exports;
		};
	}

	class MonoProcess {
	public:
		explicit MonoProcess(void* process_handle, uint32_t process_id) : process_handle_(process_handle), 
			process_id_(process_id), memory_(process_handle) {};
		~MonoProcess() {
			CloseHandle(process_handle_);
		}

		void Init();
		void GetRootDomain();
		void GetMachine();
		intptr_t OpenImageFromData(bool attach_thread, unsigned char* assembly_data, size_t assembly_size);
		intptr_t OpenAssemblyFrom(bool attach_thread, char* dll_path);
		intptr_t OpenAssemblyFromImage(bool attach_thread, intptr_t image_address);
		intptr_t GetImageFromAssembly(bool attach_thread, intptr_t assembly_address);
		intptr_t GetClassFromName(bool attach_thread, intptr_t image_address, char* name_space, char* class_name);
		intptr_t GetMethodFromName(bool attach_thread, intptr_t class_address, char* method_name);
		intptr_t GetImageLoaded(bool attach_thread, char* name);
		intptr_t GetAssemblyFromImage(bool attack_thread, intptr_t image_address);
		void CloseAssembly(bool attach_thread, intptr_t assembly_address);
		void CloseImage(bool attach_threadm, intptr_t image_address);
		void RuntimeInvoke(bool attach_thread, intptr_t method_address);

		std::vector<unsigned char> Assemble86(bool attach, intptr_t function_address,
			intptr_t result_address, intptr_t* args, size_t n_args);
		std::vector<unsigned char> Assemble64(bool attach, intptr_t function_address,
			intptr_t result_address, intptr_t* args, size_t n_args);
		std::vector<unsigned char> Assemble(bool attach, intptr_t function_address,
			intptr_t result_address, intptr_t* args, size_t n_args);
		intptr_t Execute(bool attach, intptr_t function_address, 
			intptr_t* args, size_t n_args);

	private:
		void GetMonoBaseAddress();
		void GetExports();

		static std::unordered_map<int, CachedData> cache_;
		std::unordered_map<const char*, intptr_t> exports_ = {
			{"mono_get_root_domain", 0},
			{"mono_thread_attach", 0},
			{"mono_image_open_from_data", 0},
			{"mono_assembly_load_from", 0},
			{"mono_assembly_get_image", 0},
			{"mono_class_from_name", 0},
			{"mono_class_get_method_from_name", 0},
			{"mono_runtime_invoke", 0},
			{"mono_assembly_open", 0},
			{"mono_image_loaded", 0},
			{"mono_image_get_assembly", 0},
			{"mono_assembly_close", 0},
			{"mono_image_close", 0}
		};

		const char* kMonoGetDomain = "mono_get_root_domain";
		const char* kMonoThreadAttach = "mono_thread_attach";
		const char* kMonoImageOpen = "mono_image_open_from_data";
		const char* KMonoAssemblyLoad = "mono_assembly_load_from";
		const char* kMonoAssemblyGetImage = "mono_assembly_get_image";
		const char* kMonoClassFromName = "mono_class_from_name";
		const char* kMonoClassGetMethod = "mono_class_get_method_from_name";
		const char* KMonoInvoke = "mono_runtime_invoke";
		const char* kMonoAssemblyOpen = "mono_assembly_open";
		const char* kMonoImageLoaded = "mono_image_loaded";
		const char* kMonoImageGetAssembly = "mono_image_get_assembly";
		const char* kMonoAssemblyClose = "mono_assembly_close";
		const char* kMonoImageClose = "mono_image_close";

		Memory memory_;
		void* process_handle_;
		uint32_t process_id_;
		int16_t machine_ = 0;

		intptr_t process_base_address_ = 0;
		intptr_t mono_base_address_ = 0;
		intptr_t root_domain_ = 0;
	};
	std::unordered_map<int, CachedData> MonoProcess::cache_{};
}