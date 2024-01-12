#include "pch.h"
#include "Assembler.h"

namespace Native {
	void Assembler::AppendInt32(intptr_t arg) {
		for (int i = 0; i < 4; ++i) {
			bytes.push_back(arg % 256);
			arg >>= 8;
		}
	}

	void Assembler::AppendInt64(intptr_t arg) {
		for (int i = 0; i < 8; ++i) {
			bytes.push_back(arg % 256);
			arg >>= 8;
		}
	}

	void Assembler::Push(intptr_t arg) {
		bytes.push_back(arg < 128 ? 0x6A : 0x68);
		if (arg < 256) {
			bytes.push_back((char)arg);
		}
		else {
			AppendInt32(arg);
		}
	}

	void Assembler::MovToRax(intptr_t arg) {
		bytes.push_back(0x48);
		bytes.push_back(0xB8);
		AppendInt64(arg);
	}

	void Assembler::MovToEax(intptr_t arg) {
		bytes.push_back(0xB8);
		AppendInt32(arg);
	}

	void Assembler::MovToRcx(intptr_t arg) {
		bytes.push_back(0x48);
		bytes.push_back(0xB9);
		AppendInt64(arg);//
	}

	void Assembler::MovToRdx(intptr_t arg) {
		bytes.push_back(0x48);
		bytes.push_back(0xBA);
		AppendInt64(arg);
	}

	void Assembler::MovToR8(intptr_t arg) {
		bytes.push_back(0x49);
		bytes.push_back(0xB8);
		AppendInt64(arg);
	}

	void Assembler::MovToR9(intptr_t arg) {
		bytes.push_back(0x49);
		bytes.push_back(0xB9);
		AppendInt64(arg);
	}

	void Assembler::AddRsp(char arg) {
		bytes.push_back(0x48);
		bytes.push_back(0x83);
		bytes.push_back(0xC4);
		bytes.push_back(arg);
	}

	void Assembler::AddEsp(char arg) {
		bytes.push_back(0x83);
		bytes.push_back(0xC4);
		bytes.push_back(arg);
	}

	void Assembler::SubRsp(char arg) {
		bytes.push_back(0x48);
		bytes.push_back(0x83);
		bytes.push_back(0xEC);
		bytes.push_back(arg);
	}

	void Assembler::CallRax() {
		bytes.push_back(0xFF);
		bytes.push_back(0xD0);
	}

	void Assembler::CallEax() {
		bytes.push_back(0xFF);
		bytes.push_back(0xD0);
	}

	void Assembler::MovRaxTo(intptr_t arg) {
		bytes.push_back(0x48);
		bytes.push_back(0xA3);
		AppendInt64(arg);
	}

	void Assembler::MovEaxTo(intptr_t arg) {
		bytes.push_back(0xA3);
		AppendInt32(arg);
	}

	void Assembler::Return() {
		bytes.push_back(0xC3);
	}

	char* Assembler::ToByteArray(size_t* size) {
		char* arr = new char[bytes.size()];
		for (int i = 0; i < bytes.size(); ++i)
			arr[i] = bytes[i];
		*size = bytes.size();
		return arr;
	}
}