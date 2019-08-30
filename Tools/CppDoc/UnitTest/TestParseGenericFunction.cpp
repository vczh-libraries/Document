#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericFunction)
{
	auto input = LR"(
template<typename T, typename U>
auto P(T, U)->decltype(T{}+U{});

template<typename T, int ...ts>
auto F(T t)
{
	return {t, ts...};
}
)";

	auto output = LR"(
template<typename T, typename U>
__forward P: (auto->decltype((T{} + U{}))) (T, U);
template<typename T, int ...ts>
F: auto (t: T)
{
	return {t, ts...};
}
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"P",					L"P",					L"<::P::[T], ::P::[U]> any_t __cdecl(::P::[T], ::P::[U]) * $PR"						);
	AssertExpr(pa, L"P<int, double>",		L"P<int, double>",		L"double __cdecl(__int32, double) * $PR"											);

	AssertExpr(pa, L"F",					L"F",					L"<::F::[T], ...*> any_t __cdecl(::F::[T]) * $PR"									);
	AssertExpr(pa, L"F<void*,1,2,3>",		L"F<void *, 1, 2, 3>",	L"{void * $L, __int32 $PR, __int32 $PR, __int32 $PR} __cdecl(void *) * $PR"			);
}

TEST_CASE(TestParseGenericFunction_ResolveTypeWithLocalVariables)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
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
template<typename T = int, typename U = double>
auto P(T, U)->decltype(T{}+U{});

template<typename T, int ...ts>
auto F(T t)
{
	return {t, ts...};
}
)";

	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"P<>(1,2)",					L"P<>(1, 2)",						L"double $PR"													);
	AssertExpr(pa, L"P<int>(1,2)",				L"P<int>(1, 2)",					L"double $PR"													);
	AssertExpr(pa, L"P<int, double>(1,2)",		L"P<int, double>(1, 2)",			L"double $PR"													);
	AssertExpr(pa, L"F<void*,1,2,3>(nullptr)",	L"F<void *, 1, 2, 3>(nullptr)",		L"{void * $L, __int32 $PR, __int32 $PR, __int32 $PR} $PR"		);
}

TEST_CASE(TestParseGenericFunction_CallFullInstantiated_Overloading)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}