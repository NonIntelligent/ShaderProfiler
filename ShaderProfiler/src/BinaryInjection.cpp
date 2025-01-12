#include "BinaryInjection.h"

#include <iostream>
#include <functional> // To pass member functions to QBDI callbacks
#include <iomanip>

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

	INT64* value = new INT64;

	if (result == S_OK) {
		RemoteRead(secondExit, value, sizeof(INT64));
		printf("Key is: %lli \n", (*value));
	}

	delete value;
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

QBDI::VMAction showInstruction(QBDI::VM* vm, QBDI::GPRState* gprState, QBDI::FPRState* fprState, void* data) {
	// Obtain an analysis of the instruction from the VM
	const QBDI::InstAnalysis* instAnalysis = vm->getInstAnalysis();

	// Printing disassembly
	std::cout << std::setbase(16) << instAnalysis->address << ": "
		<< instAnalysis->disassembly << std::endl << std::setbase(10);

	return QBDI::VMAction::CONTINUE;
}

void ProfilerQBDI::init() {
	_vm = new QBDI::VM();
	state = _vm->getGPRState();

	allocatedStack = QBDI::allocateVirtualStack(state, STACK_SIZE, &fakeStack);

	// Trigger callback whenever address range is executed (could be use to track entry into module)
	//vm.addCodeRangeCB(0x001, 0x002, QBDI::PREINST, ProfilerQBDI::countIteration, &iterationCount);
}


void ProfilerQBDI::destroy() {
	_vm->removeAllInstrumentedRanges();
	_vm->clearAllCache();
	QBDI::alignedFree(fakeStack);

	delete(_vm);
}

void ProfilerQBDI::addInstrumentedModule(QBDI::rword addr) {
	// Add address ranges from a remote module
	bool success = _vm->addInstrumentedModuleFromAddr(addr);

	printf("Module instrumented %d at address %p\n", success, (void*)addr);
}

void ProfilerQBDI::addInstrumentedModule(QBDI::rword start, QBDI::rword end) {
	// Add address ranges from a remote module
	_vm->addInstrumentedRange(start, end);

	printf("Module instrumented at address ranges %p --> %p\n", (void*)start, (void*) end);
}

void ProfilerQBDI::addAddressHitCount(QBDI::rword addr, void* data) {
	// Add code address for callback -> look for exact address of remote module
	uint32_t cid1 = _vm->addCodeAddrCB(addr, QBDI::PREINST, ProfilerQBDI::countIteration, data);
	uint32_t cid2 = _vm->addCodeCB(QBDI::PREINST, showInstruction, nullptr);

	switch (cid1)
	{
	case QBDI::INVALID_EVENTID :
		printf("Invalid event when adding show Instruction callback\n");
		break;
	default:
		printf("ID of registered callback event %u\n", cid1);
		printf("ID of registered callback event %u\n", cid2);
	}
}

void ProfilerQBDI::addAddressEntryCB(QBDI::rword addr) {
	_vm->addMnemonicCB("CALL*", QBDI::PREINST, ProfilerQBDI::printAddressOnEntry, nullptr);
}

void ProfilerQBDI::removeAllInstrumentation() {
	_vm->removeAllInstrumentedRanges();
}

QBDI::VMAction ProfilerQBDI::countIteration(QBDI::VMInstanceRef vm, QBDI::GPRState* gprState, QBDI::FPRState* fprState, void* data) {
	(*((size_t*)data))++;

	return QBDI::CONTINUE;
}

QBDI::VMAction ProfilerQBDI::printAddressOnEntry(QBDI::VMInstanceRef vm, QBDI::GPRState* gprState, QBDI::FPRState* fprState, void* data) {
	const QBDI::InstAnalysis* analysis = vm->getInstAnalysis(QBDI::ANALYSIS_INSTRUCTION);
	
	printf("Called function from module %s at address %p\n", analysis->moduleName, (void*) analysis->address);

	return QBDI::CONTINUE;
}
