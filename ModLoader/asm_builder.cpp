#include "stdafx.h"
#include "asm_builder.h"

namespace Injector {
	void AsmBuilder::AppendInt32(intptr_t arg) {
		for (int i = 0; i < 4; ++i) {
			bytes_.push_back(arg % 256);
			arg >>= 8;
		}
	}

	void AsmBuilder::AppendInt64(intptr_t arg) {
		for (int i = 0; i < 8; ++i) {
			bytes_.push_back(arg % 256);
			arg >>= 8;
		}
	}

	void AsmBuilder::Push(intptr_t arg) {
		bytes_.push_back(arg < 128 ? 0x6A : 0x68);
		if (arg < 256)
			bytes_.push_back((unsigned char)arg);
		else
			AppendInt32(arg);
	}

	void AsmBuilder::MovToRax(intptr_t arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0xB8);
		AppendInt64(arg);
	}

	void AsmBuilder::MovToEax(intptr_t arg) {
		bytes_.push_back(0xB8);
		AppendInt32(arg);
	}

	void AsmBuilder::MovToRcx(intptr_t arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0xB9);
		AppendInt64(arg);
	}

	void AsmBuilder::MovToRdx(intptr_t arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0xBA);
		AppendInt64(arg);
	}

	void AsmBuilder::MovToR8(intptr_t arg) {
		bytes_.push_back(0x49);
		bytes_.push_back(0xB8);
		AppendInt64(arg);
	}

	void AsmBuilder::MovToR9(intptr_t arg) {
		bytes_.push_back(0x49);
		bytes_.push_back(0xB9);
		AppendInt64(arg);
	}

	void AsmBuilder::AddRsp(unsigned char arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0x83);
		bytes_.push_back(0xC4);
		bytes_.push_back(arg);
	}

	void AsmBuilder::AddEsp(unsigned char arg) {
		bytes_.push_back(0x83);
		bytes_.push_back(0xC4);
		bytes_.push_back(arg);
	}

	void AsmBuilder::SubRsp(unsigned char arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0x83);
		bytes_.push_back(0xEC);
		bytes_.push_back(arg);
	}

	void AsmBuilder::CallRax() {
		bytes_.push_back(0xFF);
		bytes_.push_back(0xD0);
	}

	void AsmBuilder::CallEax() {
		bytes_.push_back(0xFF);
		bytes_.push_back(0xD0);
	}

	void AsmBuilder::MovRaxTo(intptr_t arg) {
		bytes_.push_back(0x48);
		bytes_.push_back(0xA3);
		AppendInt64(arg);
	}

	void AsmBuilder::MovEaxTo(intptr_t arg) {
		bytes_.push_back(0xA3);
		AppendInt32(arg);
	}

	void AsmBuilder::Return() {
		bytes_.push_back(0xC3);
	}

	std::vector<unsigned char> AsmBuilder::GetBytes() const {
		return bytes_;
	}
}