#include "BinaryInjection.h"

#include <iostream>

// Function to reverse bits of num
template <typename T>
T reverseBits(T num) {
	unsigned int NO_OF_BITS = sizeof(num) * 8;
	T reverse_num = 0;
	int i;
	for (i = 0; i < NO_OF_BITS; i++) {
		if ((num & (1 << i)))
			reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
	}
	return reverse_num;
}

bool IsPrintable(char* buf, size_t size) {
	for (size_t i = 0; i < size; i++) {
		char c = buf[i];
		if ((c == 0x09) || (c == 0x0A) || (c == 0x0D) ||
			((c >= 0x20) && (c <= 0x7E))) {
			//pass
		} else {
			return false;
		}
	}
	return true;
}

Profiler::Profiler() {
	RegisterHook(new ReadHook());
	RegisterHook(new ShaderCacheValue());
}

void Profiler::startProfiling(unsigned int pId, uint32_t timeout /*= 10000U*/) {
	if (_running) return;

	_running = true;
	_traceThread = std::thread(&Profiler::attachToProcess, this, pId, timeout);
}

void Profiler::stopProfilingAndExit() {
	if (!_running) return;
	
	_running = false;
	// TODO check last status or move instrumentation cleanup on second thread.
	_traceThread.join();
	//clearAllInstrumentation();
	//Kill();
}

void Profiler::attachToProcess(unsigned int pId, uint32_t timeout) {
	DebuggerStatus status = Attach(pId, timeout);

	while (_running && (status == DEBUGGER_HANGED || status == DEBUGGER_ATTACHED)) {
		status = Continue(timeout);
	}

	// To do, proper handling

	switch (status) {
	case DEBUGGER_ATTACHED:
	printf("Attached\n");
	break;
	case DEBUGGER_HANGED:
	printf("Hanged\n");
	break;
	case DEBUGGER_CRASHED: // Occurs when target program closes
	printf("crashed\n");
	break;
	case DEBUGGER_TARGET_START:
	printf("debug target start after attach\n");
	break;
	case DEBUGGER_PROCESS_EXIT:
	break;
	default:
		break;
	}

	shutdown();
}

void Profiler::shutdown() {
	_running = false;
	//clearAllInstrumentation();
	Kill();
}

void Profiler::clearAllInstrumentation() {
	for (ModuleInfo* modInfo : instrumented_modules) {
		modInfo->ClearInstrumentation();
	}
}

void Profiler::OnProcessExit() {
	TinyInst::OnProcessExit();
	printf("Process exiting");
}

// Only called, the first time the method is reached.
void Profiler::OnTargetMethodReached() {
	TinyInst::OnTargetMethodReached();
	printf("Reached target: Profiler::OnTargetMethodReached\n");

}

void Profiler::OnModuleUnloaded(void* module) {
	TinyInst::OnModuleUnloaded(module);
	std::cout << "Module unloaded\n";
}

void Profiler::OnCrashed(Exception* exception_record) {
	TinyInst::OnCrashed(exception_record);
	auto type = exception_record->type;
}

bool Profiler::OnException(Exception* exception_record) {
	bool result = TinyInst::OnException(exception_record);
	auto type = exception_record->type;

	return result;
}

void Profiler::OnModuleEntered(ModuleInfo* module, size_t entry_address) {
	TinyInst::OnModuleEntered(module, entry_address);
	std::string name = module->module_name;

	std::cout << "Module entered: " << name << "\n";

	std::cout << "Entry address at: " << module->module_header << "\n";
}

// Module has been instrumented on load
void Profiler::OnModuleInstrumented(ModuleInfo* module) {
	TinyInst::OnModuleInstrumented(module);
	std::cout << "Module Instrumented: " << module->module_name << std::endl;
}

void Profiler::OnModuleUninstrumented(ModuleInfo* module) {
	TinyInst::OnModuleUninstrumented(module);
	std::string name = module->module_name;
	printf("Module Uninstrumented ");
	printf(name.append("\n").c_str());
}

// Add function for on entered and store all parameters as global to check against return value
void ShaderCacheValue::OnFunctionEntered() {
	pKeyEntry = (INT64)GetArg(0);
	pKeyReverseEntry = reverseBits(pKeyEntry);

	secondEntry = (INT64*)GetArg(1); // out or target location

	keySizeEntry = (unsigned long long)GetArg(2); // Possibly true or false

	keySizePtrEntry = (UINT64*)GetArg(3); // Out params?
	keySizePtr2Entry = (INT64*)GetArg(4);
}

void ShaderCacheValue::OnFunctionReturned() {
	HRESULT result = (HRESULT)GetReturnValue();

	pKeyExit = (INT64) GetArg(0);
	pKeyReverseExit = reverseBits(pKeyExit);

	secondExit = (INT64*) GetArg(1);
	keySizeExit = (unsigned long long)GetArg(2);

	keySizePtrExit = (UINT64*) GetArg(3);
	keySizePtr2Exit = (INT64*) GetArg(4);

	unsigned char* key = new unsigned char[pKeyReverseExit];


	if (result == S_OK) {
		RemoteRead(secondExit, key, pKeyReverseExit);
		printf("Key is: %s\n", key);
	}

	delete[] key;
}

void ReadHook::OnFunctionReturned() {
	HRESULT result = (HRESULT)GetReturnValue();

	void* pKey = (void*)GetArg(0);
	int keySize1 = (int)GetArg(1);
	size_t keySize2 = (size_t)GetArg(1);
	UINT64 keySize3 = (UINT64)GetArg(1);
	unsigned short keySize4 = (unsigned short)GetArg(1);
	short keySize5 = (short)GetArg(1);
	unsigned long keySize6 = (unsigned long)GetArg(1);
	long keySize7 = (long)GetArg(1);
	UINT64 key = 0;

	if (result == S_OK) {
		//RemoteRead(pKey, &key, keySize2);
		//RemoteRead(pKey, &key, keySize2);
		printf("Key is: %llu\n", key);
	}
}
