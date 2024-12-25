#pragma once
#include <string>

enum D3dModule {
	NONE, D3DSCACHE, DXCOMPILER, D3DCOMPILER, DIRECTX11, DIRECTX12
};

struct Process {
	unsigned long id;
	unsigned long parentId;
	std::wstring name;
	D3dModule module = NONE;
	static int created;
	static int copied;
	static int deleted;

	Process() :id(0), parentId(0), name(L"default") { created++; }

	Process(unsigned long ID, unsigned long ParentId, std::wstring filename)
		:id(ID), parentId(ParentId), name(filename) { created++; }

	// Copy constructor
	Process(const Process& other) {
		id = other.id;
		parentId = other.parentId;
		name = other.name;
		module = other.module;
		copied++;
	}

	~Process() {
		deleted++;
	}

	bool isAlive() const;
};