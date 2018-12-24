#include "call_thunk.h"

#ifdef _WIN32

#include <windows.h>

#else
#include <sys/mman.h>

#ifndef offsetof
#define offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif //offsetof

#ifndef _countof
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif //_countof

#endif //_WIN32

#include <string.h>
#include <assert.h>
#include <memory>

namespace call_thunk {

#pragma pack(push, 1)

#if defined(_M_IX86) || defined(__i386__)
#include "thunk_code_x86.cpp"
#elif defined(_M_X64) || defined(__x86_64__)
#include "thunk_code_x64.cpp"
#endif 

void base_thunk::init_code(call_declare caller, call_declare callee, size_t argc, const argument_info* arginfos) throw(bad_call)
{
	_thunk_size = thunk_code::calc_size(caller, callee, argc, arginfos);

#if defined(_WIN32)
	_code = (char*)VirtualAlloc(NULL, _thunk_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
	_code = (char*)mmap(NULL, _thunk_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif //_WIN32
	_thunk = reinterpret_cast<thunk_code*>(_code);
	_code += sizeof(thunk_code);
	new(_thunk) thunk_code(caller, callee, argc, arginfos);
}

void base_thunk::destroy_code()
{
	if (_thunk) 
	{
#if defined(_WIN32)
		VirtualFree(_thunk, 0, MEM_RELEASE);
#else
		munmap(_thunk, _thunk_size);
#endif //_WIN32
		_thunk = NULL;
		_code = NULL;
		_thunk_size = 0;
	}
}

void base_thunk::flush_cache()
{
#ifdef _WIN32
	FlushInstructionCache(GetCurrentProcess(), _thunk, _thunk_size);
#else
#endif //_WIN32
}

void base_thunk::bind_impl(void* object, void* proc)
{
	_thunk->bind(object, proc);
}

}
