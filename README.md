Generally, the C function can not directly callback the member functions of the C++ class. Some C functions pass the this pointer by providing additional application data pointer parameters to solve this problem. But other C functions do not have such a design, but thunk technology can be used to solve this problem. Call_thunk is the library of the member functions that use thunk technology to callback the C++ class from the C function.
 
> Note: all the code for call_thunk is in the namespace call_thunk.

## Calling Conventions

In the 32 - bit environment of X86, the common function calling conventions are cdecl, stdcall, fastcall, and the member functions, as well as thiscall. On windows, the default calling Convention for common functions is cdecl, and the default calling convention of the member functions of the C++ class is thiscall. On Linux, all the default calling conventions for all functions are cdecl.

In the 64 bit environment of x86-64, the only function calling convention is fastcall.

Call_thunk can convert between these function calling conventions.Call_thunk supports Windows and Linux.

```C++
enum call_declare
{
	cc_fastcall,	//__FASTCALL__
	cc_cdecl,		//__CDECL__
	cc_stdcall,		//__STDCALL__
	cc_thiscall,	//__THISCALL__
};
```

## Usage

### Demo

There are the following C function declarations:
```C
typedef void (*cb_type)(int, void*);

void test_cb(cb_type cb);

```
The called C++ class is defined as follows:

```C++
struct TestClass
{
	void fun(int, void*);
};

```


### Class unsafe_thunk

Class unsafe_thunk is:

```C++
class unsafe_thunk
｛
	explicit unsafe_thunk(size_t argc, const argument_info* arginfos = NULL, call_declare caller = default_caller, call_declare callee = default_callee)；
	template<typename T, typename PROC>
	unsafe_thunk(T* object, PROC proc, size_t argc, const argument_info* arginfos = NULL, call_declare caller = default_caller, call_declare callee = default_callee)；
	template<typename T, typename PROC>
	void bind(T& object, PROC proc)；
	template<typename Callback>
	operator Callback() const；
｝；
```

Using the class unsafe_thunk is very simple to implement the member functions of the C++ class from the C function:
```C++
TestClass obj;
call_thunk::unsafe_thunk thunk(2);	//Two integer parameters
thunk.bind(obj, &TestClass::fun);
test_cb(thunk);
```

### Structure argument_info

When the parameters of the callback function are not all integers or pointers that are consistent with the length of the CPU word, when using the class unsafe_thunk, we need to provide more parameter information through second parameters. The parameter information required for thunk is defined by the structure argument_info:
```C++
struct argument_info
{
	short _size;			//Parameter size calculated in bytes
	bool _is_floating;		//Whether the parameter is floating point
};
```

### Class thunk (Only support C++11)

Class unsafe_thunk does not check the consistency of the number and type of member functions and callback functions. Therefore, if the wrong parameters are passed, it may cause the program to crash. If you use the C++11 development program, you can use the class thunk instead of unsafe_thunk. The class thunk checks whether the number and type of the parameter of the member function and the callback function are consistent, and it is simpler to use and does not need to provide parameter information.

```C++
template<typename CallerRet, typename... CallerArgs>
class thunk
{
	typedef CallerRet (*CallbackType)(CallerArgs...);

	explicit thunk(call_declare callee = default_caller);
	template<typename CalleeClass, typename CalleeProc>
	thunk(CalleeClass& object, CalleeProc proc);

	template<typename CalleeClass, typename CalleeProc,
		typename = typename std::enable_if<check_call<CalleeProc>::value>::type>
	void bind(CalleeClass& object, CalleeProc proc);
	operator CallbackType() const;
};

```

The usage is as follows:
```C++
TestClass obj;
call_thunk::thunk thunk<cb_type>(obj, &TestClass::fun);
test_cb(thunk);

```

