#pragma once

namespace Native {
	class Assembler {
		std::vector<char> bytes;
	public:
		void Push(intptr_t arg);		// push arg
		void MovToRax(intptr_t arg);	// mov rax,arg
		void MovToEax(intptr_t arg);	// mov eax,arg
		void MovToRcx(intptr_t arg);	// mov rcx,arg
		void MovToRdx(intptr_t arg);	// mov rdx,arg
		void MovToR8(intptr_t arg);	// mov r8,arg
		void MovToR9(intptr_t arg);	// mov r9,arg
		void AddRsp(char arg);	// add rsp,arg
		void AddEsp(char arg);	// add esp,arg
		void SubRsp(char arg);	// sub rsp,arg
		void CallRax();		// call rax
		void CallEax();		// call eax
		void MovRaxTo(intptr_t arg);		// mov [arg],rax
		void MovEaxTo(intptr_t arg);		// mov [arg],eax
		void AppendInt32(intptr_t arg);
		void AppendInt64(intptr_t arg);
		void Return();
		char* ToByteArray(size_t* size);
	};
}