#include "Util.h"

#pragma warning (push)
#pragma warning (disable: 4101)
#pragma warning (disable: 5046)

#define ASSERT_OVERLOADING(INPUT, OUTPUT, TYPE)\
	AssertExpr(L#INPUT, OUTPUT, L#TYPE " $PR", pa)

TEST_CASE(TestParseExpr_Overloading_Ref)
{
	{
		TEST_DECL(
struct X{};
double F(const X&);
bool F(X&);
char F(X&&);
X x;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(static_cast<const X&>(x)),		L"F(static_cast<X const &>(x))",		double);
		ASSERT_OVERLOADING(F(static_cast<X&>(x)),			L"F(static_cast<X &>(x))",				bool);
		ASSERT_OVERLOADING(F(static_cast<X&&>(x)),			L"F(static_cast<X &&>(x))",				char);
		ASSERT_OVERLOADING(F(x),							L"F(x)",								bool);
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
		TEST_DECL(
struct X{};
struct Y{};
struct Z:X{};

double F(X&);
bool F(Y&);
double G(X&);
bool G(Y&);
char G(const Z&);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
	}
}

TEST_CASE(TestParseExpr_Overloading_TypeConversion)
{
	{
		TEST_DECL(
struct X{};
struct Y{};
struct Z{ operator X(); };

double F(X);
bool F(Y);
double G(X);
bool G(Y);
char G(Z);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
	}

	{
		TEST_DECL(
struct Z{};
struct X{ X(const Z&); };
struct Y{};

double F(X);
bool F(Y);
double G(X);
bool G(Y);
char G(Z);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
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

#undef ASSERT_OVERLOADING

#pragma warning (pop)