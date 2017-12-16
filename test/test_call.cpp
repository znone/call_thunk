#include <stdio.h>
#include "call_thunk.h"

#define PARAM_LIST int a, long b, double c, int d, int e, int f

class TestClass
{
public:
	TestClass() : m(100) { }

	double do_fun(PARAM_LIST) const
	{
		double x = (a + b + c + d + e + f)*m;
		printf("hello %f", x);
		return x;
	}

	double __FASTCALL__ fastcall_fun(PARAM_LIST) const
	{
		printf("fastcall\t");
		return do_fun(a, b, c, d, e, f);
	}

	double __CDECL__ cdecl_fun(PARAM_LIST) const
	{
		printf("cdecl\t");
		return do_fun(a, b, c, d, e, f);
	}

	double __STDCALL__ stdcall_fun(PARAM_LIST) const
	{
		printf("stdcall\t");
		return do_fun(a, b, c, d, e, f);
	}

	double __THISCALL__ fun(PARAM_LIST) const
	{
		printf("thiscall\t");
		return do_fun(a, b, c, d, e, f);
	}

private:
	int m;
};

typedef double (__FASTCALL__ *fastcall_cb)(PARAM_LIST);
typedef double (__CDECL__ *cdecl_cb)(PARAM_LIST);
typedef double (__STDCALL__ *stdcall_cb)(PARAM_LIST);

extern "C"
{
	void test_fastcall_cb(fastcall_cb c)
	{
		printf("fastcall -> ");
		printf("\tok, %f\n", c(1, 2, 3.0, 4, 5, 6));
	}

	void test_cb(cdecl_cb c)
	{
		printf("cdecl -> ");
		printf("\tok, %f\n", c(1, 2, 3.0, 4, 5, 6));
	}

	void test_stdcall_cb(stdcall_cb c)
	{
		printf("stdcall -> ");
		printf("\tok, %f\n", c(1, 2, 3.0, 4, 5, 6));
	}

}

int main()
{

	TestClass a;
	{
		call_thunk::thunk<fastcall_cb> thunk(a, &TestClass::fastcall_fun);
		test_fastcall_cb(thunk);
	}

#if defined(_M_IX86) || defined(__i386__)
	{
		call_thunk::thunk<fastcall_cb> thunk(a, &TestClass::cdecl_fun);
		test_fastcall_cb(thunk);
	}

	{
		call_thunk::thunk<fastcall_cb> thunk(a, &TestClass::stdcall_fun);
		test_fastcall_cb(thunk);
	}

	{
		call_thunk::thunk<fastcall_cb> thunk(a, &TestClass::fun);
		test_fastcall_cb(thunk);
	}

	{
		call_thunk::thunk<cdecl_cb> thunk(a, &TestClass::fastcall_fun);
		test_cb(thunk);
	}

	{
		call_thunk::thunk<cdecl_cb> thunk(a, &TestClass::cdecl_fun);
		test_cb(thunk);
	}

	{
		call_thunk::thunk<cdecl_cb> thunk(a, &TestClass::stdcall_fun);
		test_cb(thunk);
	}

	{
		call_thunk::thunk<cdecl_cb> thunk(a, &TestClass::fun);
		test_cb(thunk);
	}

	{
		call_thunk::thunk<stdcall_cb> thunk(a, &TestClass::fastcall_fun);
		test_stdcall_cb(thunk);
	}

	{
		call_thunk::thunk<stdcall_cb> thunk(a, &TestClass::cdecl_fun);
		test_stdcall_cb(thunk);
	}

	{
		call_thunk::thunk<stdcall_cb> thunk(a, &TestClass::stdcall_fun);
		test_stdcall_cb(thunk);
	}

	{
		call_thunk::thunk<stdcall_cb> thunk(a, &TestClass::fun);
		test_stdcall_cb(thunk);
	}
#endif

	return 0;
}

