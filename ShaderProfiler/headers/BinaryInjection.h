#pragma once
#include <TinyInst/hook.h>
#include <TinyInst/tinyinst.h>
#include <thread>
#include <atomic>

class ShaderCacheValue : public HookBeginEnd {
public:
	INT64 pKeyEntry = 0;
	INT64 pKeyReverseEntry = 0;
	INT64* secondEntry = nullptr;
	unsigned long long keySizeEntry = 0;
	UINT64* keySizePtrEntry = nullptr;
	INT64* keySizePtr2Entry = nullptr;

	INT64 pKeyExit = 0;
	INT64 pKeyReverseExit = 0;
	INT64* secondExit = nullptr;
	unsigned long long keySizeExit = 0;
	UINT64* keySizePtrExit = nullptr;
	INT64* keySizePtr2Exit = nullptr;

public:
	ShaderCacheValue() : HookBeginEnd("*", "ShaderCache_FindValue", 5, CALLCONV_DEFAULT) {}

protected:
	void OnFunctionEntered() override;
	void OnFunctionReturned() override;
};

class ReadHook : public HookBeginEnd {
public:
	ReadHook() : HookBeginEnd("d3d13.dll", "CreateGraphicsPipelineState", 3, CALLCONV_DEFAULT) {}
protected:
	void OnFunctionReturned() override;
};

class Profiler : public TinyInst{
public:
	Profiler();

	void startProfiling(unsigned int pId, uint32_t timeout = 1000U);
	void stopProfilingAndExit();

	void clearAllInstrumentation();

	// callbacks
	void OnProcessExit() override;
	void OnTargetMethodReached() override;
	void OnModuleUnloaded(void* module) override;

	void OnCrashed(Exception* exception_record) override;
	bool OnException(Exception* exception_record) override;
	//void OnProcessExit() override;

	// Instrumentation callbacks
	void OnModuleEntered(ModuleInfo* module, size_t entry_address) override;
	void OnModuleInstrumented(ModuleInfo* module) override;
	void OnModuleUninstrumented(ModuleInfo* module) override;

	bool isRunning() const { return _running == true; }
private:
	std::atomic<bool> _running = false;
	std::thread _traceThread;

private:
	void attachToProcess(unsigned int pId, uint32_t timeout);
	void shutdown();
};