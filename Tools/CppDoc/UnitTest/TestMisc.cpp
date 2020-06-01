#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Hidden types")
	{
		auto input = LR"(
	struct A;
	struct A* A();

	enum B;
	enum B* (*B)();
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"A()",				L"A()",									L"::A * $PR"	);
		AssertExpr(pa, L"struct A()",		L"enum_class_struct_union A()",			L"::A $PR"		);
		AssertExpr(pa, L"B()",				L"B()",									L"::B * $PR"	);
		AssertExpr(pa, L"enum B()",			L"enum_class_struct_union B()",			L"::B $PR"		);
	});

	TEST_CATEGORY(L"Discarded declarations")
	{
		auto input = LR"(
template<typename T>
struct X
{
	T y;
};

template class X<int>;
template int X<int>::y;
)";
		COMPILE_PROGRAM(program, pa, input);
	});

	TEST_CATEGORY(L"Ignored declarations")
	{
		auto input = LR"(
int operator "" suffix(const char* str, int len);
)";
		COMPILE_PROGRAM(program, pa, input);
	});
}