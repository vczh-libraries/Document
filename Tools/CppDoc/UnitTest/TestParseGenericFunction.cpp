#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericFunction)
{
	auto input = LR"(
template<typename T, int ...ts>
auto F(T t)
{
	return {t, ts...};
}
)";

	auto output = LR"(
template<typename T, int ...ts>
F: auto (t: T) __cdecl
{
	return {t, ts...};
}
)";

	//COMPILE_PROGRAM(program, pa, input);
	//AssertProgram(program, output);
}

TEST_CASE(TestParseGenericFunction_ConnectForward)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_ConnectForward_Overloading)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_ConnectForward_Ambiguity)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_CallFullInstantiated)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_CallFullInstantiated_Overloading)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}