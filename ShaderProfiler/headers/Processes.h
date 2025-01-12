#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include <basetsd.h> // for dword_ptr
#include <intsafe.h> // DWORD

enum D3dModule {
	NONE, D3DSCACHE, DXCOMPILER, DIRECTX11, DIRECTX12, DIRECTX11ON12
};

struct ModuleData {
	std::wstring name;
	DWORD_PTR address = 0x0;
	DWORD dataSize = 0;
	D3dModule type = NONE;
};

struct ModuleHash {
	size_t operator()(const ModuleData& mod) const {
		size_t hash = 0;
		hash ^= std::hash<std::wstring>{}(mod.name) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<DWORD_PTR>{}(mod.address) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= std::hash<DWORD>{}(mod.dataSize) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
};

struct ModuleEquality {
	bool operator()(const ModuleData& lhs, const ModuleData& rhs) const {
		return (lhs.name == rhs.name) && (lhs.address == rhs.address) && (lhs.dataSize == rhs.dataSize);
	}
};

struct Process {
	unsigned long id;
	unsigned long parentId;
	std::wstring name;
	std::vector<ModuleData> modules;
	std::unordered_set<ModuleData, ModuleHash, ModuleEquality> modules2;
	bool isDirectXTested = false;

	Process() :id(0), parentId(0), name(L"default") {}

	Process(unsigned long ID, unsigned long ParentId, std::wstring filename)
		:id(ID), parentId(ParentId), name(filename) {}

	bool operator==(const Process& other) const {
		return (id == other.id) && (name == other.name);
	}

	bool isAlive() const;
};