#pragma once
#include "MonoProcess.h"
using namespace System;
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;

namespace ModLoader {
	public ref class MonoProcess
	{
		Native::MonoProcess* mp;
		~MonoProcess() {
			delete mp;
		}
	protected:
		!MonoProcess() {
			delete mp;
		}
	public:
		MonoProcess(int processId) : mp(new Native::MonoProcess(processId)) {}
		int LoadMod(array<System::Byte>^ assemblyBytes, int bytesLen, String^ _namespace, String^ _class, String^ _method);
		int LoadModFrom(String^ path, String^ _namespace, String^ _class, String^ _method);
		int LoadDependency(array<System::Byte>^ assemblyBytes, int bytesLen);
		int LoadDependencyFrom(String^ path);
	};
}
