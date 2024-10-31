#pragma once
#include <string>

struct Process {
	unsigned long id;
	unsigned long parentId;
	std::wstring name;
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
		copied++;
	}

	~Process() {
		deleted++;
	}

	bool isAlive();
};