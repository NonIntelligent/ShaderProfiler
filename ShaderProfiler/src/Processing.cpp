#include "Processes.h"
#include "Processing.h"
#include <algorithm>
#include <Psapi.h>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <list>
#include <strsafe.h>

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

bool isDirectXLinked(Process& proc) {
	if (proc.module != D3dModule::NONE) return true;

	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, proc.id);
	if (!handle) return false;

	HMODULE hMods[1024];
	DWORD cbNeeded;

	// Can fail if DLLs are being loaded or unloaded
	//BOOL enumerated = EnumProcessModules(handle, hMods, sizeof(HMODULE) * 1024, &cbNeeded);
	BOOL enumerated = EnumProcessModulesEx(handle, hMods, sizeof(HMODULE) * 1024, &cbNeeded, LIST_MODULES_ALL);

	// TODO check if cbNeeded is greater than size of array
	// if so then resize array and call EnumProcessModules again

	if (!enumerated) return false;

	int totalModules = cbNeeded / sizeof(HMODULE);

	bool foundModule = false;

	for (int i = 0; i < totalModules; i++) {
		TCHAR moduleName[MAX_PATH];
		if (GetModuleFileNameEx(handle, hMods[i], moduleName, sizeof(moduleName) / sizeof(TCHAR))) {
			std::wstring name = std::wstring(moduleName);

			if (name.find(L"D3DSCache.dll") != std::string::npos) {
				foundModule = true;
				proc.module = D3dModule::D3DSCACHE;
				break;
			}
			if (name.find(L"D3DCompiler") != std::string::npos) {
				foundModule = true;
				proc.module = D3dModule::D3DCOMPILER;
				break;
			}
			if (name.find(L"dxcompiler.dll") != std::string::npos) {
				foundModule = true;
				proc.module = D3dModule::DXCOMPILER;
				break;
			}
			if (name.find(L"d3d12.dll") != std::string::npos) {
				foundModule = true;
				proc.module = D3dModule::DIRECTX12;
				break;
			}
			if (name.find(L"d3d11.dll") != std::string::npos) {
				foundModule = true;
				proc.module = D3dModule::DIRECTX11;
				break;
			}
		}
	}

	CloseHandle(handle);

	return foundModule;
}

void ProcessHandler::filterProcessList() {
	if (processes.empty()) return;

	// Find root id for my process
	DWORD thisProcessId = GetCurrentProcessId();
	Process root = getProcess(L"explorer.exe");

	std::sort(processes.begin(), processes.end(), [&](Process a, Process b) {return a.id < b.id; });

	// Add the parents of every process to build a connected map
	// Does not work with games that start from launchers
	std::unordered_multimap<DWORD, DWORD> parentMap;
	for (const Process& proc : processes) {
		if (proc.parentId != 0) {
			parentMap.emplace(proc.parentId, proc.id);
		}
	}

	std::unordered_set<DWORD> whitelist;
	std::function<void(DWORD)> visit;

	// visit every parent and their children
	// every process started from explorer.exe (root) is on the whitelist
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

	//std::erase_if(processes, [&](const Process& proc) { return !whitelist.contains(proc.id); });

	// erase this application from list and its root
	std::erase_if(processes, [&](const Process& proc) { return proc.id == thisProcessId || proc.id == root.id; });

	// Erase all processes that don't have a window
	// requires a lot of resources to check if window exists on the 100+ processes
	std::erase_if(processes, [&](const Process& proc) { return findMainWindow(proc) == nullptr; });

	// Check if they use directx related DLLs
	std::erase_if(processes, [&](Process& proc) { return !isDirectXLinked(proc); });
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

// https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress
// Get address of function names for each dll at compile time
FARPROC ProcessHandler::getFuncAddress(const wchar_t* dllName, const char* funcName) {
	HMODULE dllModule = GetModuleHandle(dllName);

	bool libraryLoaded = false;

	if (dllModule == NULL) {
		LoadLibrary(dllName);
		libraryLoaded = true;
	}

	if (dllModule == NULL) return nullptr;

	FARPROC ptr = GetProcAddress(dllModule, funcName);
	/*auto e = GetLastError();

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		e,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
									  (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)"GetProcAddress") + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
					LocalSize(lpDisplayBuf) / sizeof(TCHAR),
					TEXT("%s failed with error %d: %s"),
					"GetProcAddress", e, lpMsgBuf);

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, L"ERROR", MB_OK);*/

	if (libraryLoaded) FreeLibrary(dllModule);

	return ptr;
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
