#include "Util.h"

#pragma warning (push)
#pragma warning (disable: 4101)
#pragma warning (disable: 5046)

template<typename T, typename U>
struct IntIfSameType
{
	using Type = void;
};

template<typename T>
struct IntIfSameType<T, T>
{
	using Type = int;
};

template<typename T, typename U>
void RunOverloading()
{
	typename IntIfSameType<T, U>::Type test = 0;
}

#define ASSERT_OVERLOADING(INPUT, OUTPUT, TYPE)\
	RunOverloading<TYPE, decltype(INPUT)>, \
	AssertExpr(L#INPUT, OUTPUT, L#TYPE " $PR", pa)\

TEST_CASE(TestParseExpr_Overloading_Ref)
{
	{
		TEST_DECL(
struct X {};
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
	{
		TEST_DECL(
enum A { a };
enum B { b };
enum class C { c };
enum class D { d };

bool F(A);
char F(B);
wchar_t F(C);
float F(D);
double F(int);

char G(char);
bool G(int);
double G(double);
		);
		COMPILE_PROGRAM(program, pa, input);
		
		ASSERT_OVERLOADING(F(0),							L"F(0)",								double);
		ASSERT_OVERLOADING(F(a),							L"F(a)",								bool);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								char);
		ASSERT_OVERLOADING(F(A::a),							L"F(A :: a)",							bool);
		ASSERT_OVERLOADING(F(B::b),							L"F(B :: b)",							char);
		ASSERT_OVERLOADING(F(C::c),							L"F(C :: c)",							wchar_t);
		ASSERT_OVERLOADING(F(D::d),							L"F(D :: d)",							float);

		ASSERT_OVERLOADING(G('a'),							L"G('a')",								char);
		ASSERT_OVERLOADING(G(0),							L"G(0)",								bool);
		ASSERT_OVERLOADING(G(a),							L"G(a)",								bool);
		ASSERT_OVERLOADING(G(b),							L"G(b)",								bool);
		ASSERT_OVERLOADING(G(0.0),							L"G(0.0)",								double);
	}
}

TEST_CASE(TestParseExpr_Overloading_DefaultParameter)
{
	{
		TEST_DECL(
bool F(void*);
char F(int, int);
wchar_t F(int, double = 0);
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(0),							L"F(0)",								wchar_t);
		ASSERT_OVERLOADING(F(nullptr),						L"F(nullptr)",							bool);
		ASSERT_OVERLOADING(F(0,0),							L"F(0, 0)",								char);
		ASSERT_OVERLOADING(F(0,0.0),						L"F(0, 0.0)",							wchar_t);
		ASSERT_OVERLOADING(F(0,0.0f),						L"F(0, 0.0f)",							wchar_t);
	}
}

TEST_CASE(TestParseExpr_Overloading_VariantArguments)
{
	{
		TEST_DECL(
char F(int, ...);
wchar_t F(double, ...);
float F(int, double);
double F(double, double);
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(0, 0),							L"F(0, 0)",								float);
		ASSERT_OVERLOADING(F(0, 0.0),						L"F(0, 0.0)",							float);
		ASSERT_OVERLOADING(F(0, 0.0f),						L"F(0, 0.0f)",							float);

		ASSERT_OVERLOADING(F(0.0, 0),						L"F(0.0, 0)",							double);
		ASSERT_OVERLOADING(F(0.0, 0.0),						L"F(0.0, 0.0)",							double);
		ASSERT_OVERLOADING(F(0.0, 0.0f),					L"F(0.0, 0.0f)",						double);
		
		ASSERT_OVERLOADING(F(0),							L"F(0)",								char);
		ASSERT_OVERLOADING(F(0.0),							L"F(0.0)",								wchar_t);
		ASSERT_OVERLOADING(F(0, nullptr),					L"F(0, nullptr)",						char);
		ASSERT_OVERLOADING(F(0.0, nullptr),					L"F(0.0, nullptr)",						wchar_t);
	}
}

TEST_CASE(TestParseExpr_Overloading_Inheritance)
{
	{
		TEST_DECL(
struct X {};
struct Y {};
struct Z :X {};

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
struct X {};
struct Y {};
struct Z { operator X() { throw 0; } };

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
struct Z {};
struct X { X(const Z&) {} };
struct Y {};

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