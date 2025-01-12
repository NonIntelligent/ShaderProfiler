#pragma once
#include <string>
#include <vector>

#include <dwmapi.h>
#include <TlHelp32.h>
#include "Processes.h"


class ProcessHandler {
public:
	std::vector<Process> directXProcesses;
	std::vector<Process> processes;
	FARPROC shaderAddr[1] = { getFuncAddress(L"D3DSCACHE.dll", "ShaderCache_Create") };
public:
	void findAppProcesses();
	Process getProcess(std::wstring name);
	Process getProcess(unsigned long ID);
	Process getProcessX(std::wstring name);
	Process getProcessX(unsigned long ID);

	FARPROC getFuncAddress(const wchar_t* dllName, const char* funcName);

private:
	void scanAllRunningProcesses();
	void filterProcessList();
};