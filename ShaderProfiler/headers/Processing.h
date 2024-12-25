#pragma once
#include <string>
#include <vector>

#include <dwmapi.h>
#include <TlHelp32.h>
#include "Processes.h"

#define ProcessList std::vector<Process>

class ProcessHandler {
public:
	ProcessList processes;
	FARPROC shaderAddr[1] = { getFuncAddress(L"D3DSCACHE.dll", "ShaderCache_Create") };
public:
	void findAppProcesses();
	Process getProcess(std::wstring name);
	Process getProcess(unsigned long ID);

	FARPROC getFuncAddress(const wchar_t* dllName, const char* funcName);

private:
	void scanAllRunningProcesses();
	void filterProcessList();
};