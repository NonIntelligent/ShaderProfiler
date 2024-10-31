#pragma once
#include <string>
#include <vector>

#include <dwmapi.h>
#include <TlHelp32.h>
#include "Process.h"

#define ProcessList std::vector<Process>

class ProcessHandler {
public:
	ProcessList processes;
public:
	void findAppProcesses();
	Process getProcess(std::wstring name);
	Process getProcess(unsigned long ID);
private:
	void scanAllRunningProcesses();
	void filterProcessList();
};