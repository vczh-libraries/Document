#include "Util.h"

TEST_CASE(TestMisc_HiddenType)
{
	auto input = LR"(
struct A;
struct A* A();

enum B;
enum B* (*B)();
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"A()",				L"A()",					L"::A * $PR"	);
	AssertExpr(pa, L"struct A()",		L"struct A()",			L"::A $PR"		);
	AssertExpr(pa, L"B()",				L"B()",					L"::B * $PR"	);
	AssertExpr(pa, L"enum B()",			L"struct B()",			L"::B $PR"		);
}