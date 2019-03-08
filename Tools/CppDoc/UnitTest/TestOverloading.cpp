#include "Util.h"

#pragma warning (push)
#pragma warning (disable: 5046)

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
	{
		TEST_DECL(
double F(const volatile int*);
float F(const int*);
bool F(volatile int*);
char F(int*);

extern const volatile int a[1];
extern const int b[1];
extern volatile int c[1];
extern int d[1];
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(a),							L"F(a)",								double);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								float);
		ASSERT_OVERLOADING(F(c),							L"F(c)",								bool);
		ASSERT_OVERLOADING(F(d),							L"F(d)",								char);
	}
	{
		TEST_DECL(
double F(const volatile int[]);
float F(const int[]);
bool F(volatile int[]);
char F(int[]);

extern const volatile int a[1];
extern const int b[1];
extern volatile int c[1];
extern int d[1];
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(a),							L"F(a)",								double);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								float);
		ASSERT_OVERLOADING(F(c),							L"F(c)",								bool);
		ASSERT_OVERLOADING(F(d),							L"F(d)",								char);
	}
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
struct Z : X {};

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

#define _ ,

TEST_CASE(TestParseExpr_Overloading_Universal_Initialization)
{
	{
		TEST_DECL(
struct A
{
	A() {}
	A(int) {}
	A(int _ void*) {}
};

struct B
{
	B(void*) {}
	B(void* _ int) {}
};

struct C
{
	C(A, B, bool) {}
};

struct D
{
	D(B, A, bool) {}
};

struct X
{
	X() {}
	X(int) {}
	X(double) {}
};

struct Y
{
	Y() {}
	Y(void*) {}
	Y(X) {}
};

bool F(const A&);
void F(B&&);
float F(A&);
double F(B&);
char F(C);
wchar_t F(D);
char16_t F(Y _ void*);

A a;
B b{ nullptr };
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F({}),												L"F({})",											bool);
		ASSERT_OVERLOADING(F(1),												L"F(1)",											bool);
		ASSERT_OVERLOADING(F({1}),												L"F({1})",											bool);
		ASSERT_OVERLOADING(F({{1}}),											L"F({{1}})",										bool);
		ASSERT_OVERLOADING(F({1 _ nullptr}),									L"F({1, nullptr})",									bool);
		ASSERT_OVERLOADING(F({{1} _ {nullptr}}),								L"F({{1}, {nullptr}})",								bool);
		ASSERT_OVERLOADING(F({1 _ {}}),											L"F({1, {}})",										bool);
		ASSERT_OVERLOADING(F({{1} _ {}}),										L"F({{1}, {}})",									bool);
		ASSERT_OVERLOADING(F({{} _ nullptr}),									L"F({{}, nullptr})",								bool);
		ASSERT_OVERLOADING(F({{} _ {nullptr}}),									L"F({{}, {nullptr}})",								bool);

		ASSERT_OVERLOADING(F({nullptr}),										L"F({nullptr})",									void);
		ASSERT_OVERLOADING(F({nullptr _ 1}),									L"F({nullptr, 1})",									void);
		ASSERT_OVERLOADING(F({{nullptr} _ {1}}),								L"F({{nullptr}, {1}})",								void);
		ASSERT_OVERLOADING(F({nullptr _ {}}),									L"F({nullptr, {}})",								void);
		ASSERT_OVERLOADING(F({{nullptr} _ {}}),									L"F({{nullptr}, {}})",								void);
		ASSERT_OVERLOADING(F({{} _ 1}),											L"F({{}, 1})",										void);
		ASSERT_OVERLOADING(F({{} _ {1}}),										L"F({{}, {1}})",									void);

		ASSERT_OVERLOADING(F(a),												L"F(a)",											float);
		ASSERT_OVERLOADING(F(b),												L"F(b)",											double);

		ASSERT_OVERLOADING(F({{} _ nullptr _ true}),							L"F({{}, nullptr, true})",							char);
		ASSERT_OVERLOADING(F({{} _ {nullptr _ 1} _ true}),						L"F({{}, {nullptr, 1}, true})",						char);
		ASSERT_OVERLOADING(F({{{1}, {}} _ {{} _ {1}} _ true}),					L"F({{{1}, {}}, {{}, {1}}, true})",					char);
		ASSERT_OVERLOADING(F({{{}, {nullptr}} _ {{nullptr} _ {}} _ true}),		L"F({{{}, {nullptr}}, {{nullptr}, {}}, true})",		char);

		ASSERT_OVERLOADING(F({nullptr _ {} _ true}),							L"F({nullptr, {}, true})",							wchar_t);
		ASSERT_OVERLOADING(F({{nullptr _ 1} _ {} _ true}),						L"F({{nullptr, 1}, {}, true})",						wchar_t);
		ASSERT_OVERLOADING(F({{{} _ {1}} _ {{1}, {}} _ true}),					L"F({{{}, {1}}, {{1}, {}}, true})",					wchar_t);
		ASSERT_OVERLOADING(F({{{nullptr} _ {}} _ {{}, {nullptr}} _ true}),		L"F({{{nullptr}, {}}, {{}, {nullptr}}, true})",		wchar_t);

		ASSERT_OVERLOADING(F({} _ nullptr),										L"F({}, nullptr)",									char16_t);
		ASSERT_OVERLOADING(F(nullptr _ nullptr),								L"F(nullptr, nullptr)",								char16_t);
		ASSERT_OVERLOADING(F({nullptr} _ nullptr),								L"F({nullptr}, nullptr)",							char16_t);
		ASSERT_OVERLOADING(F({{nullptr}} _ nullptr),							L"F({{nullptr}}, nullptr)",							char16_t);
		ASSERT_OVERLOADING(F({{}} _ nullptr),									L"F({{}}, nullptr)",								char16_t);
		ASSERT_OVERLOADING(F({{1}} _ nullptr),									L"F({{1}}, nullptr)",								char16_t);
		ASSERT_OVERLOADING(F({{1.0}} _ nullptr),								L"F({{1.0}}, nullptr)",								char16_t);
		ASSERT_OVERLOADING(F({{{1}}} _ nullptr),								L"F({{{1}}}, nullptr)",								char16_t);
		ASSERT_OVERLOADING(F({{{1.0}}} _ nullptr),								L"F({{{1.0}}}, nullptr)",							char16_t);
	}
}

TEST_CASE(TestParseExpr_Overloading_UI_Conv)
{
	// UI v.s. exact match
	{
		TEST_DECL(
struct S
{
	S(int) {}
};

bool F(int);
char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING(F(0),					L"F(0)",						bool);
		ASSERT_OVERLOADING(F({0}),					L"F({0})",						bool);
	}

	// UI v.s. trival conversion
	{
		TEST_DECL(
struct S
{
	S(int) {}
};

bool F(const int&);
char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING(F(0),					L"F(0)",						bool);
		ASSERT_OVERLOADING(F({0}),					L"F({0})",						bool);
	}

	// UI v.s. integral promition
	{
		TEST_DECL(
struct S
{
	S(int) {}
};

bool F(long long);
char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING(F(0),					L"F(0)",						bool);
		ASSERT_OVERLOADING(F({0}),					L"F({0})",						bool);
	}

	// UI v.s. standard conversion
	{
		TEST_DECL(
struct S
{
	S(int) {}
};

bool F(double);
char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING(F(0),					L"F(0)",						bool);
		ASSERT_OVERLOADING(F({0}),					L"F({0})",						bool);
	}

	// UI v.s. user-defined conversion
	// should be ambiguous, not able to test using ASSERT_OVERLOADING

	// UI v.s. ellipsis
	{
		TEST_DECL(
struct S
{
	S(int) {}
};

bool F(...);
char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING(F(0),					L"F(0)",						char);
		ASSERT_OVERLOADING(F({0}),					L"F({0})",						char);
	}
}

TEST_CASE(TestParseExpr_Overloading_InitializationList)
{
	// TODO
}

TEST_CASE(TestParseExpr_Overloading_Lambda)
{
	// TODO
}

TEST_CASE(TestParseExpr_Overloading_CallOP_Ctor_UI_DPVA)
{
	// TODO
	// Mix
	//   invoke operator(CallOP)
	//   constructor(Ctor)
	//   universal initialization(UI)
	//   default paramter and variant argument together(DPVA)
}

#undef _

#pragma warning (pop)