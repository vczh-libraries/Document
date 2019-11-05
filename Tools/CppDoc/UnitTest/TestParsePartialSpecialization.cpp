#include "Util.h"

TEST_CASE(TestParsePartialSpecialization_Functions)
{
	return;
	auto input = LR"(
template<typename T> void Function(T);
template<> int Function<int>(int);
template<> double Function<double>(double);
)";

	auto output = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);
}

TEST_CASE(TestParsePartialSpecialization_Classes)
{
	return;
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
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);
}

TEST_CASE(TestParsePartialSpecialization_Methods)
{
	return;
	auto input = LR"(
template<typename T>
struct Obj
{
	template<typename U>
	void Method(T, U);

	template<>
	U Method<int>(T, int);
};
)";

	auto output = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);
}