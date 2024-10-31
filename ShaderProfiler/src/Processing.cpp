#include "Process.h"
#include "Processing.h"

void ProcessHandler::scanAllRunningProcesses() {
	processes.clear();
	PROCESSENTRY32 processData;
	// Required to iterate through processes
	processData.dwSize = sizeof(PROCESSENTRY32);
	// Get a snapshot of all running processes
	HANDLE processSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	Process32First(processSnap, &processData);
	std::wstring filename;

	while (Process32Next(processSnap, &processData)) {
		// Only track/store exe files
		filename = processData.szExeFile;

		if (filename.find(L".exe") != std::string::npos) {
			processes.emplace_back(processData.th32ProcessID, processData.th32ParentProcessID, filename);
		}
	}

	CloseHandle(processSnap);
}

void ProcessHandler::filterProcessList() {
	if (processes.empty()) return;


}

void ProcessHandler::findAppProcesses() {
	scanAllRunningProcesses();
	filterProcessList();
}

Process ProcessHandler::getProcess(std::wstring name) {
	Process proc;
	for (const Process &process : processes) {
		if (process.name == name) proc = process;
	}

	return proc;
}

Process ProcessHandler::getProcess(unsigned long ID) {
	Process proc;
	for (const Process &process : processes) {
		if (process.id == ID) proc = process;
	}

	return proc;
}

bool Process::isAlive() {
	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id);

	// Replace with proper exit code handling
	if (!handle) return false;

	DWORD exitCode;
	GetExitCodeProcess(handle, &exitCode);

	CloseHandle(handle);

	if (exitCode == STILL_ACTIVE) return true;

	return false;
}

int Process::created = 0;

int Process::copied = 0;

int Process::deleted = 0;
