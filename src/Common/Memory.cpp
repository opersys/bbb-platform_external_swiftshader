// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Memory.hpp"

#include <stdlib.h>
#include "Types.hpp"
#include "Debug.hpp"

#if defined(_WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <intrin.h>
#else
	#include <sys/mman.h>
	#include <unistd.h>
#endif

#include <errno.h>
#include <memory.h>
#include <string.h>

#ifdef __ANDROID__
	#include <cutils/log.h>
#else
	#define ALOGE(...) do { } while(0)
#endif

#undef allocate
#undef deallocate

#if (defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined (_M_X64)) && !defined(__x86__)
#define __x86__
#endif

namespace sw
{
size_t memoryPageSize()
{
	static int pageSize = 0;

	if(pageSize == 0)
	{
		#if defined(_WIN32)
			SYSTEM_INFO systemInfo;
			GetSystemInfo(&systemInfo);
			pageSize = systemInfo.dwPageSize;
		#else
			pageSize = sysconf(_SC_PAGESIZE);
		#endif
	}

	return pageSize;
}

struct Allocation
{
//	size_t bytes;
	unsigned char *block;
};

inline void *allocateRaw(size_t bytes, size_t alignment)
{
	unsigned char *block = new unsigned char[bytes + sizeof(Allocation) + alignment];
	unsigned char *aligned = nullptr;

	if(block)
	{
		aligned = (unsigned char*)((uintptr_t)(block + sizeof(Allocation) + alignment - 1) & -(intptr_t)alignment);
		Allocation *allocation = (Allocation*)(aligned - sizeof(Allocation));

	//	allocation->bytes = bytes;
		allocation->block = block;
	}

	return aligned;
}

void *allocate(size_t bytes, size_t alignment)
{
	void *memory = allocateRaw(bytes, alignment);

	if(memory)
	{
		memset(memory, 0, bytes);
	}

	return memory;
}

void deallocate(void *memory)
{
	if(memory)
	{
		unsigned char *aligned = (unsigned char*)memory;
		Allocation *allocation = (Allocation*)(aligned - sizeof(Allocation));

		delete[] allocation->block;
	}
}

void *allocateExecutable(size_t bytes)
{
	size_t pageSize = memoryPageSize();

	return allocate((bytes + pageSize - 1) & ~(pageSize - 1), pageSize);
}

void markExecutable(void *memory, size_t bytes)
{
	#if defined(_WIN32)
		unsigned long oldProtection;
		VirtualProtect(memory, bytes, PAGE_EXECUTE_READ, &oldProtection);
	#else
		if (mprotect(memory, bytes, PROT_READ | PROT_EXEC) == -1)
		{
			ALOGE("mprotect failed (%s)", strerror(errno));
		#ifdef MPROTECT_FAILURE_IS_FATAL
			abort();
		#endif
		}
	#endif
}

void deallocateExecutable(void *memory, size_t bytes)
{
	#if defined(_WIN32)
		unsigned long oldProtection;
		VirtualProtect(memory, bytes, PAGE_READWRITE, &oldProtection);
	#else
		if (mprotect(memory, bytes, PROT_READ | PROT_WRITE) == -1)
		{
			ALOGE("mprotect failed (%s)", strerror(errno));
		#ifdef MPROTECT_FAILURE_IS_FATAL
			abort();
		#endif
		}
	#endif

	deallocate(memory);
}

void clear(uint16_t *memory, uint16_t element, size_t count)
{
	#if defined(_MSC_VER) && defined(__x86__)
		__stosw(memory, element, count);
	#elif defined(__GNUC__) && defined(__x86__)
		__asm__("rep stosw" : : "D"(memory), "a"(element), "c"(count));
	#else
		for(size_t i = 0; i < count; i++)
		{
			memory[i] = element;
		}
	#endif
}

void clear(uint32_t *memory, uint32_t element, size_t count)
{
	#if defined(_MSC_VER) && defined(__x86__)
		__stosd((unsigned long*)memory, element, count);
	#elif defined(__GNUC__) && defined(__x86__)
		__asm__("rep stosl" : : "D"(memory), "a"(element), "c"(count));
	#else
		for(size_t i = 0; i < count; i++)
		{
			memory[i] = element;
		}
	#endif
}

void testAllocateExecutable()
{
	void *memory = allocateExecutable(memoryPageSize());
	if (!memory)
	{
		abort();
	}
	markExecutable(memory, memoryPageSize());
	deallocateExecutable(memory, memoryPageSize());
}
}
