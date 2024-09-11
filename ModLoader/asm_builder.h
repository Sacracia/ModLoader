#pragma once
#include "stdafx.h"

namespace Injector {
	class AsmBuilder {
	public:
		void AppendInt32(intptr_t value);
		void AppendInt64(intptr_t value);

		void Push(intptr_t arg);

		void MovToRax(intptr_t arg);
		void MovToEax(intptr_t arg);
		void MovToRcx(intptr_t arg);
		void MovToRdx(intptr_t arg);
		void MovToR8(intptr_t arg);
		void MovToR9(intptr_t arg);

		void AddRsp(unsigned char arg);
		void AddEsp(unsigned char arg);
		void SubRsp(unsigned char arg);

		void CallRax();
		void CallEax();

		void MovRaxTo(intptr_t arg);
		void MovEaxTo(intptr_t arg);

		void Return();

		std::vector<unsigned char> GetBytes() const;
	private:
		std::vector<unsigned char> bytes_;
	};
}