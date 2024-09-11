#include "stdafx.h"
#include "mono_process.h"
#include "asm_builder.h"

namespace Injector {
	void MonoProcess::GetMonoBaseAddress() {
		void* snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);

		MODULEENTRY32 module_entry;
		module_entry.dwSize = sizeof MODULEENTRY32;
		Module32First(snapshot_handle, &module_entry);
		process_base_address_ = (intptr_t)module_entry.modBaseAddr;

		do {
			if (wcsncmp(module_entry.szModule, L"mono", 4) == 0) {
				mono_base_address_ = (intptr_t)module_entry.modBaseAddr;
				break;
			}
		} while (Module32Next(snapshot_handle, &module_entry));
		CloseHandle(snapshot_handle);

		#ifdef DEVELOP_MODE
		printf("mono_base_address_ = %llX\n", mono_base_address_);
		#endif

		if (mono_base_address_ == 0)
			throw std::runtime_error("Unable to get mono address");
	}

	void MonoProcess::GetMachine() {
		int32_t e_lfanew = memory_.ReadInt32(process_base_address_ + 0x3C);
		machine_ = memory_.ReadShort(process_base_address_ + e_lfanew + 0x4);
		if (machine_ != MACHINE_64 && machine_ != MACHINE_86)
			throw std::runtime_error("Unsupported machine");
	}

	void MonoProcess::GetExports() {
		int32_t e_lfanew = memory_.ReadInt32(mono_base_address_ + 0x3C);
		machine_ = memory_.ReadShort(mono_base_address_ + e_lfanew + 0x4);
		if (machine_ != MACHINE_64 && machine_ != MACHINE_86)
			throw std::runtime_error("Unsupported architecture");

		intptr_t ntHeaders = mono_base_address_ + e_lfanew;
		intptr_t optionalHeader = ntHeaders + 0x18;
		intptr_t dataDirectory = optionalHeader + (machine_ == MACHINE_64 ? 0x70 : 0x60);
		intptr_t exportDirectory = mono_base_address_ + memory_.ReadInt32(dataDirectory);

		int32_t count = memory_.ReadInt32(exportDirectory + 0x18);
		intptr_t funcs = mono_base_address_ + memory_.ReadInt32(exportDirectory + 0x1C);
		intptr_t names = mono_base_address_ + memory_.ReadInt32(exportDirectory + 0x20);
		intptr_t ordinals = mono_base_address_ + memory_.ReadInt32(exportDirectory + 0x24);
		intptr_t addr;
		int32_t offset;
		int16_t ordinal;
		char buffer[128];
		for (int32_t i = 0; i < count; ++i) {
			offset = memory_.ReadInt32(names + i * 4);
			memory_.ReadString(mono_base_address_ + offset, buffer, 32);
			ordinal = memory_.ReadShort(ordinals + i * 2);
			addr = mono_base_address_ + memory_.ReadInt32(funcs + ordinal * 4);
			if (addr == 0)
				continue;
			for (auto& pair : exports_) {
				if (strcmp(pair.first, buffer) == 0)
					pair.second = addr;
			}
		}

		#ifdef DEVELOP_MODE
		for (const auto& exp : exports_)
			printf("%s = %llX\n", exp.first, exp.second);
		#endif

		for (const auto& exp : exports_)
			if (exp.second == 0)
				throw std::runtime_error(std::format("Unable to get {} address", exp.first));
	}

	void MonoProcess::Init() {
		if (cache_.count(process_id_) > 0) {
			auto cached = cache_[process_id_];
			mono_base_address_ = cached.mono_base_address;
			machine_ = cached.machine;
			root_domain_ = cached.root_domain;
			exports_ = cached.exports;

			#ifdef DEVELOP_MODE
			printf("Cache used");
			#endif
			return;
		}

		GetMonoBaseAddress();
		GetMachine();
		GetExports();
		GetRootDomain();
		cache_[process_id_] = { mono_base_address_, machine_, root_domain_, exports_};
	}

	std::vector<unsigned char> MonoProcess::Assemble86(bool attach, intptr_t function_address,
		intptr_t result_address, intptr_t* args, size_t args_size) {
		AsmBuilder asm_builder;
		if (attach) {
			asm_builder.Push(root_domain_);
			asm_builder.MovToEax(exports_.at(kMonoThreadAttach));
			asm_builder.CallEax();
			asm_builder.AddEsp(4);
		}
		for (size_t i = args_size - 1; i >= 0 && i < args_size; i--)
			asm_builder.Push(args[i]);

		asm_builder.MovToEax(function_address);
		asm_builder.CallEax();
		asm_builder.AddEsp((unsigned char)(args_size * 4));
		asm_builder.MovEaxTo(result_address);
		asm_builder.Return();
		return asm_builder.GetBytes();
	}

	std::vector<unsigned char> MonoProcess::Assemble64(bool attach, intptr_t function_address,
		intptr_t result_address, intptr_t* args, size_t n_args) {
		AsmBuilder asm_builder;
		asm_builder.SubRsp(40);
		if (attach) {
			asm_builder.MovToRax(exports_.at(kMonoThreadAttach));
			asm_builder.MovToRcx(root_domain_);
			asm_builder.CallRax();
		}
		asm_builder.MovToRax(function_address);
		for (size_t i = 0; i < n_args; ++i) {
			if (i == 0)
				asm_builder.MovToRcx(args[i]);
			if (i == 1)
				asm_builder.MovToRdx(args[i]);
			if (i == 2)
				asm_builder.MovToR8(args[i]);
			if (i == 3)
				asm_builder.MovToR9(args[i]);
		}
		asm_builder.CallRax();
		asm_builder.AddRsp(40);
		asm_builder.MovRaxTo(result_address);
		asm_builder.Return();
		return asm_builder.GetBytes();
	}

	std::vector<unsigned char> MonoProcess::Assemble(bool attach, intptr_t function_address,
			intptr_t result_address, intptr_t* args, size_t n_args) {
		return machine_ == MACHINE_64
			? Assemble64(attach, function_address, result_address, args, n_args)
			: Assemble86(attach, function_address, result_address, args, n_args);
	}

	intptr_t MonoProcess::Execute(bool attach, intptr_t function_address, 
			intptr_t* args, size_t n_args) {
		intptr_t result_address = memory_.Allocate(machine_ == MACHINE_64 ? 8 : 4);

		auto bytes = Assemble(attach, function_address, result_address, args, n_args);
		intptr_t address = memory_.Allocate(bytes.size());
		memory_.WriteBytes(address, bytes.data(), bytes.size());

		void* thread_handle = CreateRemoteThread(process_handle_, NULL, 0, 
			(LPTHREAD_START_ROUTINE)address, NULL, NULL, NULL);
		if (thread_handle == 0)
			throw std::runtime_error("Unable to create thread");
		if (WaitForSingleObject(thread_handle, INFINITE) != 0)
			throw std::runtime_error("Unable to execute function");

		intptr_t result = machine_ == MACHINE_64 
			? (intptr_t)memory_.ReadInt64(result_address) 
			: (intptr_t)memory_.ReadInt32(result_address);
		if ((int64_t)result == 0x00000000C0000005)
			throw std::runtime_error("Access violation");
		return result;
	}

	void MonoProcess::GetRootDomain() {
		root_domain_ = Execute(false, exports_.at(kMonoGetDomain), nullptr, 0);
		if (root_domain_ == 0)
			throw std::runtime_error("Unable to get root domain");
		#ifdef DEVELOP_MODE
		printf("root_domain: %llX\n", root_domain_);
		#endif
	}

	intptr_t MonoProcess::OpenImageFromData(bool attach_thread, unsigned char* assembly_data, size_t assembly_size) {
		intptr_t status_address = memory_.Allocate(4);

		intptr_t address = memory_.Allocate(assembly_size);
		memory_.WriteBytes(address, assembly_data, assembly_size);

		intptr_t args[4] = {address, (intptr_t)assembly_size, 1, status_address};
		intptr_t raw_image = Execute(attach_thread, exports_.at(kMonoImageOpen), args, 4);

		int32_t status = memory_.ReadInt32(status_address);
		if (status != 0)
			throw std::runtime_error("Unable to open image from data");
		return raw_image;
	}

	intptr_t MonoProcess::OpenAssemblyFrom(bool attach_thread, char* dll_path) {
		intptr_t status_address = memory_.Allocate(4);

		intptr_t path_address = memory_.Allocate(strlen(dll_path) + 1);
		memory_.WriteString(path_address, dll_path);

		intptr_t params[2] = {path_address, status_address};
		intptr_t assembly = Execute(attach_thread, exports_.at(kMonoAssemblyOpen), params, 2);
		if (assembly == 0)
			throw std::runtime_error("Unable to open assembly from dll");
		return assembly;
	}

	intptr_t MonoProcess::OpenAssemblyFromImage(bool attach_thread, intptr_t image_address) {
		intptr_t status_address = memory_.Allocate(4);
		intptr_t fname_address = memory_.Allocate(1);
		intptr_t args[4] = {image_address, fname_address, status_address};
		intptr_t assembly = Execute(attach_thread, exports_.at(KMonoAssemblyLoad), args, 4);

		int32_t status = memory_.ReadInt32(status_address);
		if (status != 0)
			throw std::runtime_error("Unable to open assembly from image");
		return assembly;
	}

	intptr_t MonoProcess::GetImageFromAssembly(bool attach_thread, intptr_t assembly_address) {
		intptr_t args[1] = {assembly_address};
		intptr_t image = Execute(attach_thread, exports_.at(kMonoAssemblyGetImage), args, 1);
		
		if (image == 0)
			throw std::runtime_error("Unable to get image from assembly");
		return image;
	}

	intptr_t MonoProcess::GetClassFromName(bool attach_thread, intptr_t image_address, char* name_space, char* class_name) {
		intptr_t namespace_address = memory_.Allocate(strlen(name_space) + 1);
		memory_.WriteString(namespace_address, name_space);

		intptr_t class_name_address = memory_.Allocate(strlen(class_name) + 1);
		memory_.WriteString(class_name_address, class_name);

		intptr_t args[3] = {image_address, namespace_address, class_name_address};
		intptr_t class_address = Execute(attach_thread, exports_.at(kMonoClassFromName), args, 3);
		if (class_address == 0)
			throw std::runtime_error("Unable to get class from name");
		return class_address;
	}

	intptr_t MonoProcess::GetMethodFromName(bool attach_thread, intptr_t class_address, char* method_name) {
		intptr_t method_name_address = memory_.Allocate(strlen(method_name) + 1);
		memory_.WriteString(method_name_address, method_name);

		intptr_t args[3] = {class_address, method_name_address, 0};
		intptr_t method_address = Execute(attach_thread, exports_.at(kMonoClassGetMethod), args, 3);
		if (method_address == 0)
			throw std::runtime_error("Unable to get method from name");
		return method_address;
	}

	intptr_t MonoProcess::GetImageLoaded(bool attach_thread, char* name) {
		intptr_t name_address = memory_.Allocate(strlen(name) + 1);
		memory_.WriteString(name_address, name);

		intptr_t args[1] = { name_address };
		intptr_t image_address = Execute(attach_thread, exports_.at(kMonoImageLoaded), args, 1);
		return image_address;
	}

	intptr_t MonoProcess::GetAssemblyFromImage(bool attach_thread, intptr_t image_address) {
		intptr_t args[1] = { image_address };
		intptr_t assembly_address = Execute(attach_thread, exports_.at(kMonoImageGetAssembly), args, 1);
		if (assembly_address == 0)
			throw std::runtime_error("Unable to get assembly from image");
		return assembly_address;
	}

	void MonoProcess::CloseAssembly(bool attach_thread, intptr_t assembly_address) {
		intptr_t args[1] = { assembly_address };
		Execute(attach_thread, exports_.at(kMonoAssemblyClose), args, 1);
	}

	void MonoProcess::CloseImage(bool attach_thread, intptr_t image_address) {
		intptr_t args[1] = { image_address };
		Execute(attach_thread, exports_.at(kMonoImageClose), args, 1);
	}

	void MonoProcess::RuntimeInvoke(bool attach_thread, intptr_t method_address) {
		intptr_t exc_address = memory_.Allocate(machine_ == MACHINE_64 ? 8 : 4);

		intptr_t params[4] = { method_address, 0, 0, exc_address};
		intptr_t result = Execute(attach_thread, exports_.at(KMonoInvoke), params, 4);
		intptr_t exc = (intptr_t)memory_.ReadInt32(exc_address);
		if (exc != 0)
			throw std::runtime_error("Unable to invoke");
	}
}