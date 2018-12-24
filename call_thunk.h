#ifndef _CALL_TRUNK_H_
#define _CALL_THUNK_H_

#include <stdlib.h>
#include <stdint.h>
#include <exception>

/*
call C++ member function as C functions's callback
*/

#if __cplusplus>=201103L || (defined(_MSC_VER) && _MSC_VER>=1800)

#ifndef USE_CPP11
#define USE_CPP11
#endif //USE_CPP11

#include <type_traits>
#include <tuple>

#endif //C++11

#if !(__cplusplus>=201103L || (defined(_MSC_VER) && _MSC_VER>=1900))

#define noexcept throw()

#endif //C++11

#if defined(_M_IX86) || defined(__i386__)

#if defined(_MSC_VER)
#define __FASTCALL__	__fastcall
#define __CDECL__		__cdecl
#define __STDCALL__		__stdcall
#define __THISCALL__	__thiscall
#elif defined(__GNUC__)
#define __FASTCALL__	__attribute__((__fastcall__))
#define __CDECL__		__attribute__((__cdecl__))
#define __STDCALL__	__attribute__((__stdcall__))
#define __THISCALL__	__attribute__((__thiscall__))
#endif 

#elif defined(_M_X64) || defined(__x86_64__)

#define __FASTCALL__
#define __CDECL__
#define __STDCALL__
#define __THISCALL__

#endif 

namespace call_thunk {

class bad_call : std::exception
{
public:
	bad_call() noexcept { }
	virtual ~bad_call() noexcept { }
	virtual const char* what() const noexcept { return "call convention mismatch of function."; }
};

enum call_declare
{
	cc_fastcall,	//__FASTCALL__
#if defined(_M_IX86) || defined(__i386__)
	cc_cdecl,		//_cdecl
	cc_stdcall,		//__STDCALL__
	cc_thiscall,	//thiscall
	default_caller = cc_cdecl,
#ifdef _WIN32
	default_callee = cc_thiscall
#else
	default_callee = cc_cdecl
#endif //_MSC_VER
#elif defined(_M_X64) || defined(__x86_64__)
	default_caller = cc_fastcall,
	default_callee = cc_fastcall
#endif
};

struct thunk_code;

struct argument_info
{
	short _size;
	bool _is_floating;

	argument_info() : _size(0), _is_floating(false) { }

#ifdef USE_CPP11
	template<typename T>
	void init()
	{
		if (std::is_pointer<T>::value || std::is_reference<T>::value)
			_size = sizeof(intptr_t);
		else
			_size = sizeof(T);
		_is_floating = std::is_floating_point<T>::value;
	}
#endif //C++11

	bool as_integer() const;
	bool as_floating() const;
	short stack_size() const;
};

inline bool argument_info::as_integer() const
{
#if defined(_M_IX86) || defined(__i386__)
	return stack_size() == sizeof(intptr_t);

#elif defined(_M_X64) || defined(__x86_64__)
#if defined(_WIN32)
	return _size == sizeof(char) || _size == sizeof(short) ||
		_size == sizeof(int) || _size == sizeof(intptr_t);
#else
	return !_is_floating && stack_size() == sizeof(intptr_t);
#endif

#endif
}

inline bool argument_info::as_floating() const
{
	return _is_floating;
}

inline short argument_info::stack_size() const
{
	return (_size + sizeof(intptr_t) - 1) / sizeof(intptr_t) * sizeof(intptr_t);
}


class base_thunk
{
protected:
	base_thunk() : _code(NULL), _thunk_size(0), _thunk(NULL) { }
	~base_thunk() { destroy_code(); }

	char* _code;

	void init_code(call_declare caller, call_declare callee, size_t argc, const argument_info* arginfos=NULL) throw(bad_call);
	void destroy_code();
	void flush_cache();
	void bind_impl(void* object, void* proc);

private:
	size_t _thunk_size;
	thunk_code* _thunk;
	base_thunk(const base_thunk&);
	base_thunk& operator=(const base_thunk&);
};

class unsafe_thunk : public base_thunk
{
public:
	explicit unsafe_thunk(size_t argc, const argument_info* arginfos = NULL, call_declare caller = default_caller, call_declare callee = default_callee)
	{
		init_code(caller, callee, argc, arginfos);
	}

	template<typename T, typename PROC>
	unsafe_thunk(T* object, PROC proc, size_t argc, const argument_info* arginfos = NULL, call_declare caller = default_caller, call_declare callee = default_callee)
	{
		init_code(caller, callee, argc, arginfos);
		bind(object, proc);
	}

	template<typename T, typename PROC>
	void bind(T& object, PROC proc)
	{
		bind_impl(static_cast<void*>(&object), *(void**)&proc);
		flush_cache();
	}

	template<typename Callback>
	operator Callback() const
	{
		return reinterpret_cast<Callback>(_code);
	}
};

#ifdef USE_CPP11

template<typename Arg, typename... Others,
	typename = typename std::enable_if<sizeof...(Others) != 0>::type>
inline size_t stat_param_bytes(size_t bytes)
{
	bytes = stat_param_bytes<Arg>(bytes);
	return stat_param_bytes<Others...>(bytes);
}

template<typename Arg>
inline size_t stat_param_bytes(size_t bytes)
{
	if (std::is_pointer<Arg>::value || std::is_reference<Arg>::value)
		bytes += sizeof(intptr_t);
	else
		bytes += (sizeof(Arg) + sizeof(intptr_t)-1) / sizeof(intptr_t) * sizeof(intptr_t);
	return bytes;
}

template<typename CallerRet, typename... CallerArgs>
class thunk_impl : public base_thunk
{
public:
	thunk_impl(call_declare caller = default_caller, call_declare callee = default_caller)
	{
		argument_info arginfos[sizeof...(CallerArgs)];
		get_argument_infos(arginfos);
		init_code(caller, callee, sizeof...(CallerArgs), arginfos);
	}
	~thunk_impl() { destroy_code(); }

private:

	template<typename CalleeRet, typename... CalleeArgs>
	struct check_call_impl : public std::integral_constant<bool,
		std::is_same<CallerRet, CalleeRet>::value &&
		std::is_same<std::tuple<CallerArgs...>, std::tuple<CalleeArgs...>>::value
		> { };

	template<typename>
	struct check_call;

	#define __CALL_THUNK_CHECK_CALL__(declare, ...)  \
	template<typename CalleeRet, typename CalleeClass, typename... CalleeArgs > \
	struct check_call<CalleeRet(declare CalleeClass::*)(CalleeArgs...) __VA_ARGS__> \
		: public check_call_impl<CalleeRet, CalleeArgs...> \
	{ \
		typedef CalleeClass class_type; \
		typedef CalleeRet result_type; \
	}

	__CALL_THUNK_CHECK_CALL__(__FASTCALL__);
	__CALL_THUNK_CHECK_CALL__(__FASTCALL__, const);
	__CALL_THUNK_CHECK_CALL__(__FASTCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__(__FASTCALL__, const volatile);

#if defined(_M_IX86) || defined(__i386__)

	__CALL_THUNK_CHECK_CALL__(__CDECL__);
	__CALL_THUNK_CHECK_CALL__(__CDECL__, const);
	__CALL_THUNK_CHECK_CALL__(__CDECL__, volatile);
	__CALL_THUNK_CHECK_CALL__(__CDECL__, const volatile);

	__CALL_THUNK_CHECK_CALL__(__STDCALL__);
	__CALL_THUNK_CHECK_CALL__(__STDCALL__, const);
	__CALL_THUNK_CHECK_CALL__(__STDCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__(__STDCALL__, const volatile);

	__CALL_THUNK_CHECK_CALL__(__THISCALL__);
	__CALL_THUNK_CHECK_CALL__(__THISCALL__, const);
	__CALL_THUNK_CHECK_CALL__(__THISCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__(__THISCALL__, const volatile);

#endif //X86

	template<size_t N>
	inline void get_argument_infos(argument_info (&arginfos)[N])
	{
		check_argument_impl<0, N, CallerArgs...>(arginfos);
	}

	template<size_t I, size_t N, typename Arg, typename... Others, typename = typename std::enable_if< I != N-1 >::type>
	inline void check_argument_impl(argument_info (&arginfos)[N])
	{
		argument_info& a = arginfos[I];
		a.init<Arg>();
		check_argument_impl<I + 1, N, Others...>(arginfos);
	}

	template<size_t I, size_t N, typename Arg>
	inline void check_argument_impl(argument_info (&arginfos)[N])
	{
		argument_info& a = arginfos[I];
		a.init<Arg>();
	}

public:
	template<typename CalleeClass, typename CalleeProc,
		typename = typename std::enable_if<check_call<CalleeProc>::value>::type>
	void bind(CalleeClass& object, CalleeProc proc)
	{
		bind_impl(static_cast<check_call<CalleeProc>::class_type*>(&object), *(void**)&proc);
		flush_cache();
	}

	template<typename Callee, 
		typename = typename std::enable_if<check_call<decltype(&Callee::operator())>::value>::type>
	void bind(Callee& callee)
	{
		auto proc = &Callee::operator();
		bind_impl(&callee, *(void**)&proc);
		flush_cache();
	}

};

template<typename CallerRet>
class thunk_impl<CallerRet> : public base_thunk
{
public:
	thunk_impl(call_declare caller = default_caller, call_declare callee = default_caller)
	{
		init_code(caller, callee, 0, NULL);
	}
	~thunk_impl() { destroy_code(); }

private:

	template<typename CalleeRet>
	struct check_call_impl : public std::integral_constant<bool,
		std::is_same<CallerRet, CalleeRet>::value> 
	{ };

	template<typename>
	struct check_call;

#define __CALL_THUNK_CHECK_CALL__VOID(declare, ...)  \
	template<typename CalleeRet, typename CalleeClass> \
	struct check_call<CalleeRet(declare CalleeClass::*)() __VA_ARGS__> \
		: public check_call_impl<CalleeRet> \
	{ \
		typedef CalleeClass class_type; \
		typedef CalleeRet result_type; \
	}

	__CALL_THUNK_CHECK_CALL__VOID(__FASTCALL__);
	__CALL_THUNK_CHECK_CALL__VOID(__FASTCALL__, const);
	__CALL_THUNK_CHECK_CALL__VOID(__FASTCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__VOID(__FASTCALL__, const volatile);

#if defined(_M_IX86) || defined(__i386__)

	__CALL_THUNK_CHECK_CALL__VOID(__CDECL__);
	__CALL_THUNK_CHECK_CALL__VOID(__CDECL__, const);
	__CALL_THUNK_CHECK_CALL__VOID(__CDECL__, volatile);
	__CALL_THUNK_CHECK_CALL__VOID(__CDECL__, const volatile);

	__CALL_THUNK_CHECK_CALL__VOID(__STDCALL__);
	__CALL_THUNK_CHECK_CALL__VOID(__STDCALL__, const);
	__CALL_THUNK_CHECK_CALL__VOID(__STDCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__VOID(__STDCALL__, const volatile);

	__CALL_THUNK_CHECK_CALL__VOID(__THISCALL__);
	__CALL_THUNK_CHECK_CALL__VOID(__THISCALL__, const);
	__CALL_THUNK_CHECK_CALL__VOID(__THISCALL__, volatile);
	__CALL_THUNK_CHECK_CALL__VOID(__THISCALL__, const volatile);

#endif //X86

public:
	template<typename CalleeClass, typename CalleeProc,
		typename = typename std::enable_if<check_call<CalleeProc>::value>::type>
		void bind(CalleeClass& object, CalleeProc proc)
	{
		bind_impl(static_cast<check_call<CalleeProc>::class_type*>(&object), *(void**)&proc);
		flush_cache();
	}

	template<typename Callee,
		typename = typename std::enable_if<check_call<decltype(&Callee::operator())>::value>::type>
		void bind(Callee& callee)
	{
		auto proc = &Callee::operator();
		bind_impl(&callee, *(void**)&proc);
		flush_cache();
	}

};

template<typename>
struct get_call_declare;

#define __CALL_THUNK_GET_CALL_DECLARE__(declare, value, ...) \
	template<typename R, typename T, typename... Args> \
	struct get_call_declare<R (declare T::*)(Args...) __VA_ARGS__> \
		: std::integral_constant<call_declare, value> { }

__CALL_THUNK_GET_CALL_DECLARE__(__FASTCALL__, cc_fastcall);
__CALL_THUNK_GET_CALL_DECLARE__(__FASTCALL__, cc_fastcall, const);
__CALL_THUNK_GET_CALL_DECLARE__(__FASTCALL__, cc_fastcall, volatile);
__CALL_THUNK_GET_CALL_DECLARE__(__FASTCALL__, cc_fastcall, const volatile);

#if defined(_M_IX86) || defined(__i386__)

__CALL_THUNK_GET_CALL_DECLARE__(__CDECL__, cc_cdecl);
__CALL_THUNK_GET_CALL_DECLARE__(__CDECL__, cc_cdecl, const);
__CALL_THUNK_GET_CALL_DECLARE__(__CDECL__, cc_cdecl, volatile);
__CALL_THUNK_GET_CALL_DECLARE__(__CDECL__, cc_cdecl, const volatile);

__CALL_THUNK_GET_CALL_DECLARE__(__STDCALL__, cc_stdcall);
__CALL_THUNK_GET_CALL_DECLARE__(__STDCALL__, cc_stdcall, const);
__CALL_THUNK_GET_CALL_DECLARE__(__STDCALL__, cc_stdcall, volatile);
__CALL_THUNK_GET_CALL_DECLARE__(__STDCALL__, cc_stdcall, const volatile);

__CALL_THUNK_GET_CALL_DECLARE__(__THISCALL__, cc_thiscall);
__CALL_THUNK_GET_CALL_DECLARE__(__THISCALL__, cc_thiscall, const);
__CALL_THUNK_GET_CALL_DECLARE__(__THISCALL__, cc_thiscall, volatile);
__CALL_THUNK_GET_CALL_DECLARE__(__THISCALL__, cc_thiscall, const volatile);

#endif //X86

template<typename>
class thunk;

template<typename CallerRet, typename... CallerArgs>
class thunk<CallerRet(__FASTCALL__*)(CallerArgs...)> : 
	public thunk_impl<CallerRet, CallerArgs...>
{
public:
	typedef thunk_impl<CallerRet, CallerArgs...> base_class;
	typedef CallerRet(__FASTCALL__ *CallbackType)(CallerArgs...);

	explicit thunk(call_declare callee = default_caller) : base_class(cc_fastcall, callee) { }

	template<typename CalleeClass, typename CalleeProc>
	thunk(CalleeClass& object, CalleeProc proc)
		: base_class(cc_fastcall, get_call_declare<CalleeProc>::value)
	{
		this->bind(object, proc);
	}

	template<typename Callee>
	thunk(Callee&& callee)
		: base_class(cc_fastcall, get_call_declare<decltype(&Callee::operator())>::value)
	{
		this->bind(callee);
	}

	operator CallbackType() const
	{
		return reinterpret_cast<CallbackType>(this->_code);
	}
};

#if defined(_M_IX86) || defined(__i386__)

template<typename CallerRet, typename... CallerArgs>
class thunk<CallerRet(__CDECL__*)(CallerArgs...)> :
	public thunk_impl<CallerRet, CallerArgs...>
{
public:
	typedef thunk_impl<CallerRet, CallerArgs...> base_class;
	typedef CallerRet(__CDECL__ *CallbackType)(CallerArgs...);
	explicit thunk(call_declare callee = default_caller) : base_class(cc_cdecl, callee) { }

	template<typename CalleeClass, typename CalleeProc>
	thunk(CalleeClass& object, CalleeProc proc)
		: base_class(cc_cdecl, get_call_declare<CalleeProc>::value)
	{
		this->bind(object, proc);
	}

	template<typename Callee>
	thunk(Callee&& callee)
		: base_class(cc_fastcall, get_call_declare<decltype(&Callee::operator())>::value)
	{
		this->bind(callee);
	}

	operator CallbackType() const
	{
		return reinterpret_cast<CallbackType>(this->_code);
	}
};

template<typename CallerRet, typename... CallerArgs>
class thunk<CallerRet(__STDCALL__*)(CallerArgs...)> :
	public thunk_impl<CallerRet, CallerArgs...>
{
public:
	typedef thunk_impl<CallerRet, CallerArgs...> base_class;
	typedef CallerRet(__STDCALL__ *CallbackType)(CallerArgs...);
	explicit thunk(call_declare callee = default_caller) : base_class(cc_stdcall, callee) { }

	template<typename CalleeClass, typename CalleeProc>
	thunk(CalleeClass& object, CalleeProc proc)
		: base_class(cc_stdcall, get_call_declare<CalleeProc>::value)
	{
		this->bind(object, proc);
	}

	template<typename Callee>
	thunk(Callee&& callee)
		: base_class(cc_fastcall, get_call_declare<decltype(&Callee::operator())>::value)
	{
		this->bind(callee);
	}

	operator CallbackType() const
	{
		return reinterpret_cast<CallbackType>(this->_code);
	}
};

#endif //X86

#endif //C++11

}

#endif //_CALL_TRUNK_H_
