#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T> void Function(T){}
template<> int Function<int>(int);
template<> double Function<double>(double);
)";

	auto output = LR"(
template<typename T>
Function: void (T)
{
}
template<>
__forward Function<int>: int (int);
template<>
__forward Function<double>: double (double);
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	TEST_CATEGORY(L"Value aliases")
	{
		auto input = LR"(
template<typename T> auto Zero = (T)0;
template<> auto Zero<char> = '0';
template<> auto Zero<wchar_t> = L'0';
)";

		auto output = LR"(
template<typename T>
using_value Zero: auto = c_cast<T>(0);
template<>
using_value Zero<char>: auto = '0';
template<>
using_value Zero<wchar_t>: auto = L'0';
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	TEST_CATEGORY(L"Classes")
	{
		auto input = LR"(
template<typename T>
struct Obj
{
};

template<typename T>
struct Obj<T*>
{
};

template<typename T, typename U>
struct Obj<T(*)(U)>
{
};
)";

		auto output = LR"(
template<typename T>
struct Obj
{
};
template<typename T>
struct Obj<T *>
{
};
template<typename T, typename U>
struct Obj<T (U) *>
{
};
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	TEST_CATEGORY(L"Methods")
	{
		auto input = LR"(
template<typename T>
struct Obj
{
	template<typename U>
	void Method(T, U);

	template<>
	int Method<int>(T, int);
};
)";

		auto output = LR"(
template<typename T>
struct Obj
{
	public template<typename U>
	__forward Method: void (T, U);
	public template<>
	__forward Method<int>: int (T, int);
};
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});
}