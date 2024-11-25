#include "Process.h"
#include "Processing.h"
#include <algorithm>
#include <Psapi.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <list>

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
		filename = processData.szExeFile;

		processes.emplace_back(processData.th32ProcessID, processData.th32ParentProcessID, filename);
	}

	CloseHandle(processSnap);
}
struct HandleData{
	unsigned long process_id = 0;
	HWND window_handle = nullptr;
};

BOOL isMainWindow(HWND handle) {
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam) {
	HandleData& data = *(HandleData*)lParam; // what in the sorcery
	unsigned long processId = 0;
	bool isnull = data.window_handle == nullptr;
	GetWindowThreadProcessId(handle, &processId);
	if (data.process_id != processId || !isMainWindow(handle))
		return true;
	data.window_handle = handle;
	return false;

}

HWND findMainWindow(const Process& proc) {

	HandleData data;

	data.process_id = proc.id;

	EnumWindows(enum_windows_callback, (LPARAM)&data);

	return data.window_handle;
}

bool isD3DCompilerLinked(const Process& proc) {
	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc.id);
	if (!handle) return false;

	HMODULE hMods[1024];
	DWORD cbNeeded;

	// Can fail if DLLs are being loaded or unloaded
	BOOL enumerated = EnumProcessModules(handle, hMods, sizeof(HMODULE) * 1024, &cbNeeded);

	// TODO check if cbNeeded is greater than size of array
	// if so then resize array and call EnumProcessModules again

	if (!enumerated) return false;

	int totalModules = cbNeeded / sizeof(HMODULE);

	bool foundModule = false;

	for (int i = 0; i < totalModules; i++) {
		TCHAR moduleName[MAX_PATH];
		if (GetModuleFileNameEx(handle, hMods[i], moduleName, sizeof(moduleName) / sizeof(TCHAR))) {
			std::wstring name = std::wstring(moduleName);
			if (name.find(L"D3DCompiler_47.dll") != std::string::npos) {
				foundModule = true; 
			}
			if (name.find(L"dxc.dll") != std::string::npos) {
				foundModule = true;
			}
			if (name.find(L"dxcompiler.dll") != std::string::npos) {
				foundModule = true;
			}
			if (name.find(L"d3d12.dll") != std::string::npos) {
				foundModule = true;
			}
		}
	}

	// Close Handle not necessary


	return foundModule;
}

void ProcessHandler::filterProcessList() {
	if (processes.empty()) return;

	// Find root id for my process
	DWORD thisProcessId = GetCurrentProcessId();
	Process root = getProcess(thisProcessId);
	Process temp = root;

	while (temp.parentId != 0) {
		root = temp;
		temp = getProcess(temp.parentId);
	}

	// Add the parents of every process to build a connected map
	std::unordered_multimap<DWORD, DWORD> parentMap;
	for (const Process& proc : processes) {
		if (proc.parentId != 0) {
			parentMap.emplace(proc.parentId, proc.id);
		}
	}

	std::unordered_set<DWORD> whitelist;
	std::function<void(DWORD)> visit;

	// visit every parent and their children
	visit = [&](DWORD root) {
		whitelist.emplace(root);
		const std::pair it = parentMap.equal_range(root);

		// While we haven't reached the end (first == begin, second == end)
		for (auto i = it.first; i != it.second; i++) {
			visit(i->second);
		}
	};

	// start from this app
	visit(root.id);

	std::erase_if(processes, [&](const Process& proc) { return !whitelist.contains(proc.id); });

	// erase this application from list and its root
	std::erase_if(processes, [&](const Process& proc) { return proc.id == thisProcessId || proc.id == root.id; });

	// Erase all processes that don't have a window
	// requires a lot of resources to check if window exists on the 100+ processes
	std::erase_if(processes, [&](const Process& proc) { return findMainWindow(proc) == nullptr; });

	// Check if they use directx related DLLs
	//std::erase_if(processes, [&](const Process& proc) { return !isD3DCompilerLinked(proc); });
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

bool Process::isAlive() const {
	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, id);

	// Replace with proper exit code handling
	if (!handle) return false;

	DWORD exitCode;
	GetExitCodeProcess(handle, &exitCode);

	CloseHandle(handle);

	return exitCode == STILL_ACTIVE;
}

int Process::created = 0;

int Process::copied = 0;

int Process::deleted = 0;
