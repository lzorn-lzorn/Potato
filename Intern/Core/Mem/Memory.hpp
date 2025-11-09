#pragma once

namespace Core::Memory{

struct IMemory{

	virtual ~IMemory() = 0;
	virtual void* Allocate(size_t size) = 0;
	virtual void Deallocate(void* ptr) = 0;
};

}