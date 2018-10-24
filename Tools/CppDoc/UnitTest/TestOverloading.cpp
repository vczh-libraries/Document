#include "Util.h"

TEST_CASE(TestParseExpr_Overloading_Ref)
{
	{
		auto input = LR"(
	struct X{};
	double F(const X&);
	bool F(X&);
	char F(X&&);
	X x;
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(L"F(static_cast<const X&>(x))",		L"F(static_cast<X const &>(x))",		L"double $PR",		pa);
		AssertExpr(L"F(static_cast<X&>(x))",			L"F(static_cast<X &>(x))",				L"bool $PR",		pa);
		AssertExpr(L"F(static_cast<X&&>(x))",			L"F(static_cast<X &&>(x))",				L"char $PR",		pa);
		AssertExpr(L"F(x)",								L"F(x)",								L"bool $PR",		pa);
	}
}

TEST_CASE(TestParseExpr_Overloading_Array)
{
}

TEST_CASE(TestParseExpr_Overloading_EnumItem)
{
}

TEST_CASE(TestParseExpr_Overloading_DefaultParameter)
{
}

TEST_CASE(TestParseExpr_Overloading_VariantArguments)
{
}

TEST_CASE(TestParseExpr_Overloading_Inheritance)
{
	{
		auto input = LR"(
	struct X{};
	struct Y{};
	struct Z:X{};

	double F(X&);
	bool F(Y&);

	double G(X&);
	bool G(Y&);
	char G(const Z&);

	Z z;
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(L"F(z)", L"F(z)", L"double $PR", pa);
		AssertExpr(L"G(z)", L"G(z)", L"char $PR", pa);
	}
}

TEST_CASE(TestParseExpr_Overloading_TypeConversion)
{
	{
		auto input = LR"(
	struct X{};
	struct Y{};
	struct Z{ operator X(); };

	double F(X);
	bool F(Y);

	double G(X);
	bool G(Y);
	char G(Z);

	Z z;
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(L"F(z)", L"F(z)", L"double $PR", pa);
		AssertExpr(L"G(z)", L"G(z)", L"char $PR", pa);
	}

	{
		auto input = LR"(
	struct Z{};
	struct X{ X(const Z&); };
	struct Y{};

	double F(X);
	bool F(Y);

	double G(X);
	bool G(Y);
	char G(Z);

	Z z;
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(L"F(z)", L"F(z)", L"double $PR", pa);
		AssertExpr(L"G(z)", L"G(z)", L"char $PR", pa);
	}
}

TEST_CASE(TestParseExpr_Overloading_PostfixUnary)
{
}

TEST_CASE(TestParseExpr_Overloading_PrefixUnary)
{
}

TEST_CASE(TestParseExpr_Overloading_Binary)
{
}

TEST_CASE(TestParseExpr_Overloading_Binary_Assignment)
{
}

TEST_CASE(TestParseExpr_Overloading_Universal_Initialization)
{
}

TEST_CASE(TestParseExpr_Overloading_Lambda)
{
}

TEST_CASE(TestParseExpr_Overloading_CallOP_Ctor_UI_DPVA)
{
}