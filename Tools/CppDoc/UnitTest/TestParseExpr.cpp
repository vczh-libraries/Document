#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseExpr_Literal)
{
	AssertExpr(L"true",			L"true",		L"bool $PR"					);
	AssertExpr(L"false",		L"false",		L"bool $PR"					);
	AssertExpr(L"nullptr",		L"nullptr",		L"nullptr_t $PR"			);

	AssertExpr(L"0",			L"0",			L"0 $PR"					);
	AssertExpr(L"0u",			L"0u",			L"0 $PR"					);
	AssertExpr(L"0U",			L"0U",			L"0 $PR"					);
	AssertExpr(L"0l",			L"0l",			L"0 $PR"					);
	AssertExpr(L"0L",			L"0L",			L"0 $PR"					);
	AssertExpr(L"0ul",			L"0ul",			L"0 $PR"					);
	AssertExpr(L"0uL",			L"0uL",			L"0 $PR"					);
	AssertExpr(L"0Ul",			L"0Ul",			L"0 $PR"					);
	AssertExpr(L"0UL",			L"0UL",			L"0 $PR"					);

	AssertExpr(L"1",			L"1",			L"__int32 $PR"				);
	AssertExpr(L"1u",			L"1u",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1U",			L"1U",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1l",			L"1l",			L"__int64 $PR"				);
	AssertExpr(L"1L",			L"1L",			L"__int64 $PR"				);
	AssertExpr(L"1ul",			L"1ul",			L"unsigned __int64 $PR"		);
	AssertExpr(L"1uL",			L"1uL",			L"unsigned __int64 $PR"		);
	AssertExpr(L"1Ul",			L"1Ul",			L"unsigned __int64 $PR"		);
	AssertExpr(L"1UL",			L"1UL",			L"unsigned __int64 $PR"		);

	AssertExpr(L"0x1",			L"0x1",			L"__int32 $PR"				);
	AssertExpr(L"0x1u",			L"0x1u",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0x1U",			L"0x1U",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0x1l",			L"0x1l",		L"__int64 $PR"				);
	AssertExpr(L"0x1L",			L"0x1L",		L"__int64 $PR"				);
	AssertExpr(L"0X1ul",		L"0X1ul",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1uL",		L"0X1uL",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1Ul",		L"0X1Ul",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1UL",		L"0X1UL",		L"unsigned __int64 $PR"		);

	AssertExpr(L"0b1",			L"0b1",			L"__int32 $PR"				);
	AssertExpr(L"0b1u",			L"0b1u",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0b1U",			L"0b1U",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0b1l",			L"0b1l",		L"__int64 $PR"				);
	AssertExpr(L"0b1L",			L"0b1L",		L"__int64 $PR"				);
	AssertExpr(L"0B1ul",		L"0B1ul",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1uL",		L"0B1uL",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1Ul",		L"0B1Ul",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1UL",		L"0B1UL",		L"unsigned __int64 $PR"		);
	
	AssertExpr(L"1.0",			L"1.0",			L"double $PR"				);
	AssertExpr(L"1.0f",			L"1.0f",		L"float $PR"				);
	AssertExpr(L"1.0F",			L"1.0F",		L"float $PR"				);
	AssertExpr(L"1.0l",			L"1.0l",		L"double $PR"				);
	AssertExpr(L"1.0L",			L"1.0L",		L"double $PR"				);

	AssertExpr(L"'x'",			L"'x'",			L"char $PR"					);
	AssertExpr(L"L'x'",			L"L'x'",		L"wchar_t $PR"				);
	AssertExpr(L"u'x'",			L"u'x'",		L"char16_t $PR"				);
	AssertExpr(L"U'x'",			L"U'x'",		L"char32_t $PR"				);
	AssertExpr(L"u8'x'",		L"u8'x'",		L"char $PR"					);

	AssertExpr(L"\"x\"",		L"\"x\"",		L"char const [] & $L"		);
	AssertExpr(L"L\"x\"",		L"L\"x\"",		L"wchar_t const [] & $L"	);
	AssertExpr(L"u\"x\"",		L"u\"x\"",		L"char16_t const [] & $L"	);
	AssertExpr(L"U\"x\"",		L"U\"x\"",		L"char32_t const [] & $L"	);
	AssertExpr(L"u8\"x\"",		L"u8\"x\"",		L"char const [] & $L"		);
}

TEST_CASE(TestParseExpr_Name)
{
	auto input = LR"(
namespace a
{
	struct X
	{
		struct Y
		{
		};

		static Y u1;
		Y u2;
		static int v1(Y&) {}
		int v2(Y&) {}
	};
}
namespace b
{
	using namespace a;
}
namespace c
{
	using namespace b;

	struct Z : X
	{
	};
}

using namespace c;

Z z1;
int z2(Z);
)";
	COMPILE_PROGRAM(program, pa, input);
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"z1", 0, 0, VariableDeclaration, 30, 2)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"z1",			L"z1",					L"::c::Z $L",								pa);
		TEST_ASSERT(accessed.Count() == 1);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"z2", 0, 2, ForwardFunctionDeclaration, 31, 4)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"::z2",			L"__root :: z2",		L"__int32 (::c::Z) * $PR",					pa);
		TEST_ASSERT(accessed.Count() == 1);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 0, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"u1", 0, 3, ForwardVariableDeclaration, 9, 11)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"Z::u1",		L"Z :: u1",				L"::a::X::Y $L",							pa);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 2, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"u2", 0, 5, VariableDeclaration, 10, 4)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"::Z::u2",		L"__root :: Z :: u2",	L"::a::X::Y (::a::X ::) * $PR",				pa);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 0, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"v1", 0, 3, FunctionDeclaration, 11, 13)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"Z::v1",		L"Z :: v1",				L"__int32 (::a::X::Y &) * $PR",				pa);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 2, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"v2", 0, 5, FunctionDeclaration, 12, 6)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"::Z::v2",		L"__root :: Z :: v2",	L"__int32 (::a::X::Y &) (::a::X ::) * $PR",	pa);
		TEST_ASSERT(accessed.Count() == 2);
	}
}

TEST_CASE(TestParseExpr_FFA)
{
	auto input = LR"(
struct X
{
	int x;
	int y;
};
struct Y
{
	double x;
	double y;

	X* operator->();
};
struct Z
{
	bool x;
	bool y;

	Y operator->();
	X operator()(int);
	Y operator()(void*);
	X operator[](const char*);
	Y operator[](Z&&);

	static int F(double);
	int G(void*);
};

Z z;
Z* pz = nullptr;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"pz->x",					L"pz->x",						L"bool $L",					pa);
	AssertExpr(L"pz->y",					L"pz->y",						L"bool $L",					pa);
	AssertExpr(L"pz->F",					L"pz->F",						L"__int32 (double) * $PR",	pa);
	AssertExpr(L"pz->G",					L"pz->G",						L"__int32 (void *) * $PR",	pa);
	AssertExpr(L"pz->operator->",			L"pz->operator ->",				L"::Y () * $PR",			pa);

	AssertExpr(L"z.x",						L"z.x",							L"bool $L",					pa);
	AssertExpr(L"z.y",						L"z.y",							L"bool $L",					pa);
	AssertExpr(L"z.F",						L"z.F",							L"__int32 (double) * $PR",	pa);
	AssertExpr(L"z.G",						L"z.G",							L"__int32 (void *) * $PR",	pa);
	AssertExpr(L"z.operator->",				L"z.operator ->",				L"::Y () * $PR",			pa);
	
	AssertExpr(L"pz->operator->()",			L"pz->operator ->()",			L"::Y $PR",					pa);
	AssertExpr(L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::X $PR",					pa);
	AssertExpr(L"pz->operator()(nullptr)",	L"pz->operator ()(nullptr)",	L"::Y $PR",					pa);
	AssertExpr(L"pz->operator[](\"a\")",	L"pz->operator [](\"a\")",		L"::X $PR",					pa);
	AssertExpr(L"pz->operator[](Z())",		L"pz->operator [](Z())",		L"::Y $PR",					pa);
	AssertExpr(L"pz->F(0)",					L"pz->F(0)",					L"__int32 $PR",				pa);
	AssertExpr(L"pz->G(0)",					L"pz->G(0)",					L"__int32 $PR",				pa);
	
	AssertExpr(L"z.operator->()",			L"z.operator ->()",				L"::Y $PR",					pa);
	AssertExpr(L"z.operator()(0)",			L"z.operator ()(0)",			L"::X $PR",					pa);
	AssertExpr(L"z.operator()(nullptr)",	L"z.operator ()(nullptr)",		L"::Y $PR",					pa);
	AssertExpr(L"z.operator[](\"a\")",		L"z.operator [](\"a\")",		L"::X $PR",					pa);
	AssertExpr(L"z.operator[](Z())",		L"z.operator [](Z())",			L"::Y $PR",					pa);
	AssertExpr(L"z.F(0)",					L"z.F(0)",						L"__int32 $PR",				pa);
	AssertExpr(L"z.G(0)",					L"z.G(0)",						L"__int32 $PR",				pa);
	
	AssertExpr(L"z->x",						L"z->x",						L"__int32 $L",				pa);
	AssertExpr(L"z->y",						L"z->y",						L"__int32 $L",				pa);
	AssertExpr(L"z(0)",						L"z(0)",						L"::X $PR",					pa);
	AssertExpr(L"z(nullptr)",				L"z(nullptr)",					L"::Y $PR",					pa);
	AssertExpr(L"z[\"a\"]",					L"z[\"a\"]",					L"::X $PR",					pa);
	AssertExpr(L"z[Z()]",					L"z[Z()]",						L"::Y $PR",					pa);
}

TEST_CASE(TestParseExpr_Field_Qualifier)
{
	auto input = LR"(
struct X
{
	int x;
	int y;
};

X x;
X& lx;
X&& rx;

const X cx;
const X& clx;
const X&& crx;

X F();
X& lF();
X&& rF();

const X cF();
const X& clF();
const X&& crF();

X* px;
const X* cpx;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"x",						L"x",							L"::X $L",					pa);
	AssertExpr(L"lx",						L"lx",							L"::X & $L",				pa);
	AssertExpr(L"rx",						L"rx",							L"::X && $L",				pa);
	AssertExpr(L"cx",						L"cx",							L"::X const $L",			pa);
	AssertExpr(L"clx",						L"clx",							L"::X const & $L",			pa);
	AssertExpr(L"crx",						L"crx",							L"::X const && $L",			pa);

	AssertExpr(L"x.x",						L"x.x",							L"__int32 $L",				pa);
	AssertExpr(L"lx.x",						L"lx.x",						L"__int32 $L",				pa);
	AssertExpr(L"rx.x",						L"rx.x",						L"__int32 $L",				pa);
	AssertExpr(L"cx.x",						L"cx.x",						L"__int32 const $L",		pa);
	AssertExpr(L"clx.x",					L"clx.x",						L"__int32 const $L",		pa);
	AssertExpr(L"crx.x",					L"crx.x",						L"__int32 const $L",		pa);

	AssertExpr(L"F().x",					L"F().x",						L"__int32 $PR",				pa);
	AssertExpr(L"lF().x",					L"lF().x",						L"__int32 $L",				pa);
	AssertExpr(L"rF().x",					L"rF().x",						L"__int32 && $X",			pa);
	AssertExpr(L"cF().x",					L"cF().x",						L"__int32 const $PR",		pa);
	AssertExpr(L"clF().x",					L"clF().x",						L"__int32 const $L",		pa);
	AssertExpr(L"crF().x",					L"crF().x",						L"__int32 const && $X",		pa);
	
	AssertExpr(L"px",						L"px",							L"::X * $L",				pa);
	AssertExpr(L"cpx",						L"cpx",							L"::X const * $L",			pa);
	AssertExpr(L"px->x",					L"px->x",						L"__int32 $L",				pa);
	AssertExpr(L"cpx->x",					L"cpx->x",						L"__int32 const $L",		pa);
}

TEST_CASE(TestParseExpr_ArrayQualifier)
{
	auto input = LR"(
int x[1];
int (&lx)[1];
int (&&rx)[1];

const int cx[1];
const int (&clx)[1];
const int (&&crx)[1];

int (&lF())[1];
int (&&rF())[1];

const int (&clF())[1];
const int (&&crF())[1];

int* px;
const int* cpx;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"x[0]",						L"x[0]",						L"__int32 & $L",			pa);
	AssertExpr(L"lx[0]",					L"lx[0]",						L"__int32 & $L",			pa);
	AssertExpr(L"rx[0]",					L"rx[0]",						L"__int32 & $L",			pa);
	AssertExpr(L"cx[0]",					L"cx[0]",						L"__int32 const & $L",		pa);
	AssertExpr(L"clx[0]",					L"clx[0]",						L"__int32 const & $L",		pa);
	AssertExpr(L"crx[0]",					L"crx[0]",						L"__int32 const & $L",		pa);

	AssertExpr(L"lF()[0]",					L"lF()[0]",						L"__int32 & $L",			pa);
	AssertExpr(L"rF()[0]",					L"rF()[0]",						L"__int32 && $X",			pa);
	AssertExpr(L"clF()[0]",					L"clF()[0]",					L"__int32 const & $L",		pa);
	AssertExpr(L"crF()[0]",					L"crF()[0]",					L"__int32 const && $X",		pa);

	AssertExpr(L"px[0]",					L"px[0]",						L"__int32 & $L",			pa);
	AssertExpr(L"cpx[0]",					L"cpx[0]",						L"__int32 const & $L",		pa);
}

TEST_CASE(TestParseExpr_FFA_Qualifier)
{
	auto input = LR"(
struct X
{
	int x;
	int y;
};
struct Y
{
	double x;
	double y;
};
struct Z
{
	X* operator->()const;
	const Y* operator->();
	X operator()(int)const;
	Y operator()(int);
	X operator[](int)const;
	Y operator[](int);
};

Z z;
const Z cz;

Z& lz;
const Z& lcz;

Z&& rz;
const Z&& rcz;

Z* const pz;
const Z* const pcz;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"z.operator->()",			L"z.operator ->()",				L"::Y const * $PR",		pa);
	AssertExpr(L"z.operator()(0)",			L"z.operator ()(0)",			L"::Y $PR",				pa);
	AssertExpr(L"z.operator[](0)",			L"z.operator [](0)",			L"::Y $PR",				pa);
	AssertExpr(L"z->x",						L"z->x",						L"double const $L",		pa);
	AssertExpr(L"z(0)",						L"z(0)",						L"::Y $PR",				pa);
	AssertExpr(L"z[0]",						L"z[0]",						L"::Y $PR",				pa);

	AssertExpr(L"cz.operator->()",			L"cz.operator ->()",			L"::X * $PR",			pa);
	AssertExpr(L"cz.operator()(0)",			L"cz.operator ()(0)",			L"::X $PR",				pa);
	AssertExpr(L"cz.operator[](0)",			L"cz.operator [](0)",			L"::X $PR",				pa);
	AssertExpr(L"cz->x",					L"cz->x",						L"__int32 $L",			pa);
	AssertExpr(L"cz(0)",					L"cz(0)",						L"::X $PR",				pa);
	AssertExpr(L"cz[0]",					L"cz[0]",						L"::X $PR",				pa);

	AssertExpr(L"lz.operator->()",			L"lz.operator ->()",			L"::Y const * $PR",		pa);
	AssertExpr(L"lz.operator()(0)",			L"lz.operator ()(0)",			L"::Y $PR",				pa);
	AssertExpr(L"lz.operator[](0)",			L"lz.operator [](0)",			L"::Y $PR",				pa);
	AssertExpr(L"lz->x",					L"lz->x",						L"double const $L",		pa);
	AssertExpr(L"lz(0)",					L"lz(0)",						L"::Y $PR",				pa);
	AssertExpr(L"lz[0]",					L"lz[0]",						L"::Y $PR",				pa);

	AssertExpr(L"lcz.operator->()",			L"lcz.operator ->()",			L"::X * $PR",			pa);
	AssertExpr(L"lcz.operator()(0)",		L"lcz.operator ()(0)",			L"::X $PR",				pa);
	AssertExpr(L"lcz.operator[](0)",		L"lcz.operator [](0)",			L"::X $PR",				pa);
	AssertExpr(L"lcz->x",					L"lcz->x",						L"__int32 $L",			pa);
	AssertExpr(L"lcz(0)",					L"lcz(0)",						L"::X $PR",				pa);
	AssertExpr(L"lcz[0]",					L"lcz[0]",						L"::X $PR",				pa);

	AssertExpr(L"rz.operator->()",			L"rz.operator ->()",			L"::Y const * $PR",		pa);
	AssertExpr(L"rz.operator()(0)",			L"rz.operator ()(0)",			L"::Y $PR",				pa);
	AssertExpr(L"rz.operator[](0)",			L"rz.operator [](0)",			L"::Y $PR",				pa);
	AssertExpr(L"rz->x",					L"rz->x",						L"double const $L",		pa);
	AssertExpr(L"rz(0)",					L"rz(0)",						L"::Y $PR",				pa);
	AssertExpr(L"rz[0]",					L"rz[0]",						L"::Y $PR",				pa);

	AssertExpr(L"rcz.operator->()",			L"rcz.operator ->()",			L"::X * $PR",			pa);
	AssertExpr(L"rcz.operator()(0)",		L"rcz.operator ()(0)",			L"::X $PR",				pa);
	AssertExpr(L"rcz.operator[](0)",		L"rcz.operator [](0)",			L"::X $PR",				pa);
	AssertExpr(L"rcz->x",					L"rcz->x",						L"__int32 $L",			pa);
	AssertExpr(L"rcz(0)",					L"rcz(0)",						L"::X $PR",				pa);
	AssertExpr(L"rcz[0]",					L"rcz[0]",						L"::X $PR",				pa);

	AssertExpr(L"pz->operator->()",			L"pz->operator ->()",			L"::Y const * $PR",		pa);
	AssertExpr(L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::Y $PR",				pa);
	AssertExpr(L"pz->operator[](0)",		L"pz->operator [](0)",			L"::Y $PR",				pa);

	AssertExpr(L"pcz->operator->()",		L"pcz->operator ->()",			L"::X * $PR",			pa);
	AssertExpr(L"pcz->operator()(0)",		L"pcz->operator ()(0)",			L"::X $PR",				pa);
	AssertExpr(L"pcz->operator[](0)",		L"pcz->operator [](0)",			L"::X $PR",				pa);
}

TEST_CASE(TestParseExpr_FFA_Qualifier_OfExplicitOrImplicitThisExpr)
{
}

TEST_CASE(TestParseExpr_AddressOfArrayFunctionMemberPointer)
{
	auto input = LR"(
struct X
{
	static int A;
	int B;
	static int C();
	int D();
};

struct Y : X
{
};

int E();

auto			_A1 = &Y::A;
auto			_B1 = &Y::B;
auto			_C1 = &Y::C;
auto			_D1 = &Y::D;
auto			_E1 = &E;

decltype(auto)	_A2 = &Y::A;
decltype(auto)	_B2 = &Y::B;
decltype(auto)	_C2 = &Y::C;
decltype(auto)	_D2 = &Y::D;
decltype(auto)	_E2 = &E;

decltype(&Y::A)	_A3[1] = &Y::A;
decltype(&Y::B)	_B3[1] = &Y::B;
decltype(&Y::C)	_C3[1] = &Y::C;
decltype(&Y::D)	_D3[1] = &Y::D;
decltype(&E)	_E3[1] = &E;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"Y::A",			L"Y :: A",				L"__int32 $L",						pa);
	AssertExpr(L"Y::B",			L"Y :: B",				L"__int32 (::X ::) * $PR",			pa);
	AssertExpr(L"Y::C",			L"Y :: C",				L"__int32 () * $PR",				pa);
	AssertExpr(L"Y::D",			L"Y :: D",				L"__int32 () (::X ::) * $PR",		pa);
	AssertExpr(L"E",			L"E",					L"__int32 () * $PR",				pa);

	AssertExpr(L"&Y::A",		L"(& Y :: A)",			L"__int32 * $PR",					pa);
	AssertExpr(L"&Y::B",		L"(& Y :: B)",			L"__int32 (::X ::) * $PR",			pa);
	AssertExpr(L"&Y::C",		L"(& Y :: C)",			L"__int32 () * $PR",				pa);
	AssertExpr(L"&Y::D",		L"(& Y :: D)",			L"__int32 () (::X ::) * $PR",		pa);
	AssertExpr(L"&E",			L"(& E)",				L"__int32 () * $PR",				pa);

	AssertExpr(L"_A1",			L"_A1",					L"__int32 * $L",					pa);
	AssertExpr(L"_B1",			L"_B1",					L"__int32 (::X ::) * $L",			pa);
	AssertExpr(L"_C1",			L"_C1",					L"__int32 () * $L",					pa);
	AssertExpr(L"_D1",			L"_D1",					L"__int32 () (::X ::) * $L",		pa);
	AssertExpr(L"_E1",			L"_E1",					L"__int32 () * $L",					pa);

	AssertExpr(L"&_A1",			L"(& _A1)",				L"__int32 * * $PR",					pa);
	AssertExpr(L"&_B1",			L"(& _B1)",				L"__int32 (::X ::) * * $PR",		pa);
	AssertExpr(L"&_C1",			L"(& _C1)",				L"__int32 () * * $PR",				pa);
	AssertExpr(L"&_D1",			L"(& _D1)",				L"__int32 () (::X ::) * * $PR",		pa);
	AssertExpr(L"&_E1",			L"(& _E1)",				L"__int32 () * * $PR",				pa);

	AssertExpr(L"_A2",			L"_A2",					L"__int32 * $L",					pa);
	AssertExpr(L"_B2",			L"_B2",					L"__int32 (::X ::) * $L",			pa);
	AssertExpr(L"_C2",			L"_C2",					L"__int32 () * $L",					pa);
	AssertExpr(L"_D2",			L"_D2",					L"__int32 () (::X ::) * $L",		pa);
	AssertExpr(L"_E2",			L"_E2",					L"__int32 () * $L",					pa);

	AssertExpr(L"&_A2",			L"(& _A2)",				L"__int32 * * $PR",					pa);
	AssertExpr(L"&_B2",			L"(& _B2)",				L"__int32 (::X ::) * * $PR",		pa);
	AssertExpr(L"&_C2",			L"(& _C2)",				L"__int32 () * * $PR",				pa);
	AssertExpr(L"&_D2",			L"(& _D2)",				L"__int32 () (::X ::) * * $PR",		pa);
	AssertExpr(L"&_E2",			L"(& _E2)",				L"__int32 () * * $PR",				pa);

	AssertExpr(L"_A3",			L"_A3",					L"__int32 * [] $L",					pa);
	AssertExpr(L"_B3",			L"_B3",					L"__int32 (::X ::) * [] $L",		pa);
	AssertExpr(L"_C3",			L"_C3",					L"__int32 () * [] $L",				pa);
	AssertExpr(L"_D3",			L"_D3",					L"__int32 () (::X ::) * [] $L",		pa);
	AssertExpr(L"_E3",			L"_E3",					L"__int32 () * [] $L",				pa);

	AssertExpr(L"&_A3",			L"(& _A3)",				L"__int32 * [] * $PR",				pa);
	AssertExpr(L"&_B3",			L"(& _B3)",				L"__int32 (::X ::) * [] * $PR",		pa);
	AssertExpr(L"&_C3",			L"(& _C3)",				L"__int32 () * [] * $PR",			pa);
	AssertExpr(L"&_D3",			L"(& _D3)",				L"__int32 () (::X ::) * [] * $PR",	pa);
	AssertExpr(L"&_E3",			L"(& _E3)",				L"__int32 () * [] * $PR",			pa);
}

TEST_CASE(TestParseExpr_DeclType_Var)
{
	auto input = LR"(
const int		x;
auto			a1 = 0;
decltype(auto)	b1 = 0;
decltype(0)		c1 = 0;
auto			a2 = x;
decltype(auto)	b2 = x;
decltype(x)		c2 = x;
auto			a3 = (x);
decltype(auto)	b3 = (x);
decltype((x))	c3 = (x);
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"a1",			L"a1",				L"__int32 $L",						pa);
	AssertExpr(L"b1",			L"b1",				L"__int32 $L",						pa);
	AssertExpr(L"c1",			L"c1",				L"__int32 $L",						pa);
	AssertExpr(L"a2",			L"a2",				L"__int32 $L",						pa);
	AssertExpr(L"b2",			L"b2",				L"__int32 const $L",				pa);
	AssertExpr(L"c2",			L"c2",				L"__int32 const $L",				pa);
	AssertExpr(L"a3",			L"a3",				L"__int32 $L",						pa);
	AssertExpr(L"b3",			L"b3",				L"__int32 const & $L",				pa);
	AssertExpr(L"c3",			L"c3",				L"__int32 const & $L",				pa);
}

TEST_CASE(TestParseExpr_DeclType_Func)
{
}

TEST_CASE(TestParseExpr_EnumAndEnumItem)
{
	auto input = LR"(
enum Season
{
	Spring,
	Summer,
	Autumn,
	Winter,
};

enum class SeasonClass
{
	Spring,
	Summer,
	Autumn,
	Winter,
};
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"Spring",					L"Spring",					L"::Season $PR",						pa);
	AssertExpr(L"Summer",					L"Summer",					L"::Season $PR",						pa);
	AssertExpr(L"Autumn",					L"Autumn",					L"::Season $PR",						pa);
	AssertExpr(L"Winter",					L"Winter",					L"::Season $PR",						pa);
	AssertExpr(L"Season::Spring",			L"Season :: Spring",		L"::Season $PR",						pa);
	AssertExpr(L"Season::Summer",			L"Season :: Summer",		L"::Season $PR",						pa);
	AssertExpr(L"Season::Autumn",			L"Season :: Autumn",		L"::Season $PR",						pa);
	AssertExpr(L"Season::Winter",			L"Season :: Winter",		L"::Season $PR",						pa);
	AssertExpr(L"SeasonClass::Spring",		L"SeasonClass :: Spring",	L"::SeasonClass $PR",					pa);
	AssertExpr(L"SeasonClass::Summer",		L"SeasonClass :: Summer",	L"::SeasonClass $PR",					pa);
	AssertExpr(L"SeasonClass::Autumn",		L"SeasonClass :: Autumn",	L"::SeasonClass $PR",					pa);
	AssertExpr(L"SeasonClass::Winter",		L"SeasonClass :: Winter",	L"::SeasonClass $PR",					pa);
}

TEST_CASE(TestParseExpr_FieldReference)
{
}

TEST_CASE(TestParseExpr_Ternary_Comma)
{
}

TEST_CASE(TestParseExpr_MISC)
{
	auto input = LR"(
namespace std
{
	struct type_info{};
}
using namespace std;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"typeid(int)",					L"typeid(int)",							L"::std::type_info $L",			pa);
	AssertExpr(L"typeid(0)",					L"typeid(0)",							L"::std::type_info $L",			pa);
	AssertExpr(L"sizeof(int)",					L"sizeof(int)",							L"unsigned __int32 $PR",		pa);
	AssertExpr(L"sizeof 0",						L"sizeof(0)",							L"unsigned __int32 $PR",		pa);
	AssertExpr(L"throw 0",						L"throw(0)",							L"void $PR",					pa);
																						
	AssertExpr(L"new type_info",				L"new type_info()",						L"::std::type_info * $PR",		pa);
	AssertExpr(L"new type_info(0)",				L"new type_info(0)",					L"::std::type_info * $PR",		pa);
	AssertExpr(L"new(0)type_info",				L"new (0) type_info()",					L"::std::type_info * $PR",		pa);
	AssertExpr(L"new(0,1)type_info(0,1)",		L"new (0, 1) type_info(0, 1)",			L"::std::type_info * $PR",		pa);
	AssertExpr(L"new type_info[10]",			L"new type_info[10]",					L"::std::type_info * $PR",		pa);
	AssertExpr(L"new (0)type_info[10]",			L"new (0) type_info[10]",				L"::std::type_info * $PR",		pa);
	AssertExpr(L"new (0,1)type_info[10,20]",	L"new (0, 1) type_info[(10 , 20)]",		L"::std::type_info * $PR",		pa);
																						
	AssertExpr(L"delete 0",						L"delete (0)",							L"void $PR",					pa);
	AssertExpr(L"delete [] 0",					L"delete[] (0)",						L"void $PR",					pa);
}

TEST_CASE(TestParseExpr_Universal_Initialization)
{
}

TEST_CASE(TestParseExpr_Lambda)
{
	// TsysType::Lambda
	// TsysType::CapturedLambda
	// GetElement() returns a function type
	// GetDecl() returns the scope inside the lambda
}