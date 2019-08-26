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
	AssertExpr(L"1l",			L"1l",			L"__int32 $PR"				);
	AssertExpr(L"1L",			L"1L",			L"__int32 $PR"				);
	AssertExpr(L"1ll",			L"1ll",			L"__int64 $PR"				);
	AssertExpr(L"1LL",			L"1LL",			L"__int64 $PR"				);
	AssertExpr(L"1ul",			L"1ul",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1uL",			L"1uL",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1Ul",			L"1Ul",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1UL",			L"1UL",			L"unsigned __int32 $PR"		);
	AssertExpr(L"1ull",			L"1ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"1uLL",			L"1uLL",		L"unsigned __int64 $PR"		);
	AssertExpr(L"1Ull",			L"1Ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"1ULL",			L"1ULL",		L"unsigned __int64 $PR"		);

	AssertExpr(L"0x1",			L"0x1",			L"__int32 $PR"				);
	AssertExpr(L"0x1u",			L"0x1u",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0x1U",			L"0x1U",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0x1l",			L"0x1l",		L"__int32 $PR"				);
	AssertExpr(L"0x1L",			L"0x1L",		L"__int32 $PR"				);
	AssertExpr(L"0x1ll",		L"0x1ll",		L"__int64 $PR"				);
	AssertExpr(L"0x1LL",		L"0x1LL",		L"__int64 $PR"				);
	AssertExpr(L"0X1ul",		L"0X1ul",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0X1uL",		L"0X1uL",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0X1Ul",		L"0X1Ul",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0X1UL",		L"0X1UL",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0X1ull",		L"0X1ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1uLL",		L"0X1uLL",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1Ull",		L"0X1Ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0X1ULL",		L"0X1ULL",		L"unsigned __int64 $PR"		);

	AssertExpr(L"0b1",			L"0b1",			L"__int32 $PR"				);
	AssertExpr(L"0b1u",			L"0b1u",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0b1U",			L"0b1U",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0b1l",			L"0b1l",		L"__int32 $PR"				);
	AssertExpr(L"0b1L",			L"0b1L",		L"__int32 $PR"				);
	AssertExpr(L"0b1ll",		L"0b1ll",		L"__int64 $PR"				);
	AssertExpr(L"0b1LL",		L"0b1LL",		L"__int64 $PR"				);
	AssertExpr(L"0B1ul",		L"0B1ul",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0B1uL",		L"0B1uL",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0B1Ul",		L"0B1Ul",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0B1UL",		L"0B1UL",		L"unsigned __int32 $PR"		);
	AssertExpr(L"0B1ull",		L"0B1ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1uLL",		L"0B1uLL",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1Ull",		L"0B1Ull",		L"unsigned __int64 $PR"		);
	AssertExpr(L"0B1ULL",		L"0B1ULL",		L"unsigned __int64 $PR"		);

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
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"z1", 0, 1, VariableDeclaration, 30, 2)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" z1",			L"z1",						L"::c::Z $L"											);
		AssertExpr(pa, L"&z1",			L"(& z1)",					L"::c::Z * $PR"											);
		TEST_ASSERT(accessed.Count() == 1);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"z2", 0, 3, ForwardFunctionDeclaration, 31, 4)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" ::z2",		L"__root :: z2",			L"__int32 __cdecl(::c::Z) * $PR"						);
		AssertExpr(pa, L"&::z2",		L"(& __root :: z2)",		L"__int32 __cdecl(::c::Z) * $PR"						);
		TEST_ASSERT(accessed.Count() == 1);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"Z", 0, 1, ClassDeclaration, 23, 8)
			ASSERT_SYMBOL(1, L"u1", 0, 4, ForwardVariableDeclaration, 9, 11)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" Z::u1",		L"Z :: u1",					L"::a::X::Y $L"											);
		AssertExpr(pa, L"&Z::u1",		L"(& Z :: u1)",				L"::a::X::Y * $PR"										);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"Z", 0, 3, ClassDeclaration, 23, 8)
			ASSERT_SYMBOL(1, L"u2", 0, 6, VariableDeclaration, 10, 4)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" ::Z::u2",		L"__root :: Z :: u2",		L"::a::X::Y $L"											);
		AssertExpr(pa, L"&::Z::u2",		L"(& __root :: Z :: u2)",	L"::a::X::Y (::a::X ::) * $PR"							);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"Z", 0, 1, ClassDeclaration, 23, 8)
			ASSERT_SYMBOL(1, L"v1", 0, 4, FunctionDeclaration, 11, 13)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" Z::v1",		L"Z :: v1",					L"__int32 __cdecl(::a::X::Y &) * $PR"					);
		AssertExpr(pa, L"&Z::v1",		L"(& Z :: v1)",				L"__int32 __cdecl(::a::X::Y &) * $PR"					);
		TEST_ASSERT(accessed.Count() == 2);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL(0, L"Z", 0, 3, ClassDeclaration, 23, 8)
			ASSERT_SYMBOL(1, L"v2", 0, 6, FunctionDeclaration, 12, 6)
		END_ASSERT_SYMBOL;

		AssertExpr(pa, L" ::Z::v2",		L"__root :: Z :: v2",		L"__int32 __thiscall(::a::X::Y &) * $PR"				);
		AssertExpr(pa, L"&::Z::v2",		L"(& __root :: Z :: v2)",	L"__int32 __thiscall(::a::X::Y &) (::a::X ::) * $PR"	);
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

X (*x)(int) = nullptr;
Y (*const y)(int) = nullptr;

X (*(*x2)(void*))(int) = nullptr;
Y (*const (*const y2)(void*))(int) = nullptr;

Z z;
Z* pz = nullptr;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"pz->x",					L"pz->x",						L"bool $L"								);
	AssertExpr(pa, L"pz->y",					L"pz->y",						L"bool $L"								);
	AssertExpr(pa, L"pz->F",					L"pz->F",						L"__int32 __cdecl(double) * $PR"		);
	AssertExpr(pa, L"pz->G",					L"pz->G",						L"__int32 __thiscall(void *) * $PR"		);
	AssertExpr(pa, L"pz->operator->",			L"pz->operator ->",				L"::Y __thiscall() * $PR"				);

	AssertExpr(pa, L"z.x",						L"z.x",							L"bool $L"								);
	AssertExpr(pa, L"z.y",						L"z.y",							L"bool $L"								);
	AssertExpr(pa, L"z.F",						L"z.F",							L"__int32 __cdecl(double) * $PR"		);
	AssertExpr(pa, L"z.G",						L"z.G",							L"__int32 __thiscall(void *) * $PR"		);
	AssertExpr(pa, L"z.operator->",				L"z.operator ->",				L"::Y __thiscall() * $PR"				);

	AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::Y $PR"								);
	AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::X $PR"								);
	AssertExpr(pa, L"pz->operator()(nullptr)",	L"pz->operator ()(nullptr)",	L"::Y $PR"								);
	AssertExpr(pa, L"pz->operator[](\"a\")",	L"pz->operator [](\"a\")",		L"::X $PR"								);
	AssertExpr(pa, L"pz->operator[](Z())",		L"pz->operator [](Z())",		L"::Y $PR"								);
	AssertExpr(pa, L"pz->F(0)",					L"pz->F(0)",					L"__int32 $PR"							);
	AssertExpr(pa, L"pz->G(0)",					L"pz->G(0)",					L"__int32 $PR"							);

	AssertExpr(pa, L"z.operator->()",			L"z.operator ->()",				L"::Y $PR"								);
	AssertExpr(pa, L"z.operator()(0)",			L"z.operator ()(0)",			L"::X $PR"								);
	AssertExpr(pa, L"z.operator()(nullptr)",	L"z.operator ()(nullptr)",		L"::Y $PR"								);
	AssertExpr(pa, L"z.operator[](\"a\")",		L"z.operator [](\"a\")",		L"::X $PR"								);
	AssertExpr(pa, L"z.operator[](Z())",		L"z.operator [](Z())",			L"::Y $PR"								);
	AssertExpr(pa, L"z.F(0)",					L"z.F(0)",						L"__int32 $PR"							);
	AssertExpr(pa, L"z.G(0)",					L"z.G(0)",						L"__int32 $PR"							);

	AssertExpr(pa, L"z->x",						L"z->x",						L"__int32 $L"							);
	AssertExpr(pa, L"z->y",						L"z->y",						L"__int32 $L"							);
	AssertExpr(pa, L"z(0)",						L"z(0)",						L"::X $PR"								);
	AssertExpr(pa, L"z(nullptr)",				L"z(nullptr)",					L"::Y $PR"								);
	AssertExpr(pa, L"z[\"a\"]",					L"z[\"a\"]",					L"::X $PR"								);
	AssertExpr(pa, L"z[Z()]",					L"z[Z()]",						L"::Y $PR"								);

	AssertExpr(pa, L"x(0)",						L"x(0)",						L"::X $PR"								);
	AssertExpr(pa, L"y(0)",						L"y(0)",						L"::Y $PR"								);
	AssertExpr(pa, L"x2(nullptr)(0)",			L"x2(nullptr)(0)",				L"::X $PR"								);
	AssertExpr(pa, L"y2(nullptr)(0)",			L"y2(nullptr)(0)",				L"::Y $PR"								);
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

	AssertExpr(pa, L"x",						L"x",							L"::X $L"					);
	AssertExpr(pa, L"lx",						L"lx",							L"::X & $L"					);
	AssertExpr(pa, L"rx",						L"rx",							L"::X && $L"				);
	AssertExpr(pa, L"cx",						L"cx",							L"::X const $L"				);
	AssertExpr(pa, L"clx",						L"clx",							L"::X const & $L"			);
	AssertExpr(pa, L"crx",						L"crx",							L"::X const && $L"			);

	AssertExpr(pa, L"x.x",						L"x.x",							L"__int32 $L"				);
	AssertExpr(pa, L"lx.x",						L"lx.x",						L"__int32 $L"				);
	AssertExpr(pa, L"rx.x",						L"rx.x",						L"__int32 $L"				);
	AssertExpr(pa, L"cx.x",						L"cx.x",						L"__int32 const $L"			);
	AssertExpr(pa, L"clx.x",					L"clx.x",						L"__int32 const $L"			);
	AssertExpr(pa, L"crx.x",					L"crx.x",						L"__int32 const $L"			);

	AssertExpr(pa, L"F().x",					L"F().x",						L"__int32 $PR"				);
	AssertExpr(pa, L"lF().x",					L"lF().x",						L"__int32 $L"				);
	AssertExpr(pa, L"rF().x",					L"rF().x",						L"__int32 && $X"			);
	AssertExpr(pa, L"cF().x",					L"cF().x",						L"__int32 const $PR"		);
	AssertExpr(pa, L"clF().x",					L"clF().x",						L"__int32 const $L"			);
	AssertExpr(pa, L"crF().x",					L"crF().x",						L"__int32 const && $X"		);

	AssertExpr(pa, L"px",						L"px",							L"::X * $L"					);
	AssertExpr(pa, L"cpx",						L"cpx",							L"::X const * $L"			);
	AssertExpr(pa, L"px->x",					L"px->x",						L"__int32 $L"				);
	AssertExpr(pa, L"cpx->x",					L"cpx->x",						L"__int32 const $L"			);
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

	AssertExpr(pa, L"x[0]",						L"x[0]",						L"__int32 & $L"				);
	AssertExpr(pa, L"lx[0]",					L"lx[0]",						L"__int32 & $L"				);
	AssertExpr(pa, L"rx[0]",					L"rx[0]",						L"__int32 & $L"				);
	AssertExpr(pa, L"cx[0]",					L"cx[0]",						L"__int32 const & $L"		);
	AssertExpr(pa, L"clx[0]",					L"clx[0]",						L"__int32 const & $L"		);
	AssertExpr(pa, L"crx[0]",					L"crx[0]",						L"__int32 const & $L"		);

	AssertExpr(pa, L"lF()[0]",					L"lF()[0]",						L"__int32 & $L"				);
	AssertExpr(pa, L"rF()[0]",					L"rF()[0]",						L"__int32 && $X"			);
	AssertExpr(pa, L"clF()[0]",					L"clF()[0]",					L"__int32 const & $L"		);
	AssertExpr(pa, L"crF()[0]",					L"crF()[0]",					L"__int32 const && $X"		);

	AssertExpr(pa, L"px[0]",					L"px[0]",						L"__int32 & $L"				);
	AssertExpr(pa, L"cpx[0]",					L"cpx[0]",						L"__int32 const & $L"		);
}

TEST_CASE(TestParseExpr_FFA_Qualifier)
{
	return;
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

	AssertExpr(pa, L"z.operator->()",			L"z.operator ->()",				L"::Y const * $PR"			);
	AssertExpr(pa, L"z.operator()(0)",			L"z.operator ()(0)",			L"::Y $PR"					);
	AssertExpr(pa, L"z.operator[](0)",			L"z.operator [](0)",			L"::Y $PR"					);
	AssertExpr(pa, L"z->x",						L"z->x",						L"double const $L"			);
	AssertExpr(pa, L"z(0)",						L"z(0)",						L"::Y $PR"					);
	AssertExpr(pa, L"z[0]",						L"z[0]",						L"::Y $PR"					);

	AssertExpr(pa, L"cz.operator->()",			L"cz.operator ->()",			L"::X * $PR"				);
	AssertExpr(pa, L"cz.operator()(0)",			L"cz.operator ()(0)",			L"::X $PR"					);
	AssertExpr(pa, L"cz.operator[](0)",			L"cz.operator [](0)",			L"::X $PR"					);
	AssertExpr(pa, L"cz->x",					L"cz->x",						L"__int32 $L"				);
	AssertExpr(pa, L"cz(0)",					L"cz(0)",						L"::X $PR"					);
	AssertExpr(pa, L"cz[0]",					L"cz[0]",						L"::X $PR"					);

	AssertExpr(pa, L"lz.operator->()",			L"lz.operator ->()",			L"::Y const * $PR"			);
	AssertExpr(pa, L"lz.operator()(0)",			L"lz.operator ()(0)",			L"::Y $PR"					);
	AssertExpr(pa, L"lz.operator[](0)",			L"lz.operator [](0)",			L"::Y $PR"					);
	AssertExpr(pa, L"lz->x",					L"lz->x",						L"double const $L"			);
	AssertExpr(pa, L"lz(0)",					L"lz(0)",						L"::Y $PR"					);
	AssertExpr(pa, L"lz[0]",					L"lz[0]",						L"::Y $PR"					);

	AssertExpr(pa, L"lcz.operator->()",			L"lcz.operator ->()",			L"::X * $PR"				);
	AssertExpr(pa, L"lcz.operator()(0)",		L"lcz.operator ()(0)",			L"::X $PR"					);
	AssertExpr(pa, L"lcz.operator[](0)",		L"lcz.operator [](0)",			L"::X $PR"					);
	AssertExpr(pa, L"lcz->x",					L"lcz->x",						L"__int32 $L"				);
	AssertExpr(pa, L"lcz(0)",					L"lcz(0)",						L"::X $PR"					);
	AssertExpr(pa, L"lcz[0]",					L"lcz[0]",						L"::X $PR"					);

	AssertExpr(pa, L"rz.operator->()",			L"rz.operator ->()",			L"::Y const * $PR"			);
	AssertExpr(pa, L"rz.operator()(0)",			L"rz.operator ()(0)",			L"::Y $PR"					);
	AssertExpr(pa, L"rz.operator[](0)",			L"rz.operator [](0)",			L"::Y $PR"					);
	AssertExpr(pa, L"rz->x",					L"rz->x",						L"double const $L"			);
	AssertExpr(pa, L"rz(0)",					L"rz(0)",						L"::Y $PR"					);
	AssertExpr(pa, L"rz[0]",					L"rz[0]",						L"::Y $PR"					);

	AssertExpr(pa, L"rcz.operator->()",			L"rcz.operator ->()",			L"::X * $PR"				);
	AssertExpr(pa, L"rcz.operator()(0)",		L"rcz.operator ()(0)",			L"::X $PR"					);
	AssertExpr(pa, L"rcz.operator[](0)",		L"rcz.operator [](0)",			L"::X $PR"					);
	AssertExpr(pa, L"rcz->x",					L"rcz->x",						L"__int32 $L"				);
	AssertExpr(pa, L"rcz(0)",					L"rcz(0)",						L"::X $PR"					);
	AssertExpr(pa, L"rcz[0]",					L"rcz[0]",						L"::X $PR"					);

	AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::Y const * $PR"			);
	AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::Y $PR"					);
	AssertExpr(pa, L"pz->operator[](0)",		L"pz->operator [](0)",			L"::Y $PR"					);

	AssertExpr(pa, L"pcz->operator->()",		L"pcz->operator ->()",			L"::X * $PR"				);
	AssertExpr(pa, L"pcz->operator()(0)",		L"pcz->operator ()(0)",			L"::X $PR"					);
	AssertExpr(pa, L"pcz->operator[](0)",		L"pcz->operator [](0)",			L"::X $PR"					);
}

Symbol* GetFunctionBodyStatementSymbol(const ParsingArguments& pa, const WString& className, const WString& methodName)
{
	auto classSymbol = pa.context->TryGetChildren_NFb(className)->Get(0);
	auto functionSymbol = classSymbol->TryGetChildren_NFb(methodName)->Get(0);
	auto functionBodySymbol = functionSymbol->GetImplSymbols_F()[0];
	return functionBodySymbol->TryGetChildren_NFb(L"$")->Get(0).Obj();
}

TEST_CASE(TestParseExpr_FFA_Qualifier_OfExplicitOrImplicitThisExpr)
{
	return;
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

	void M(){}
	void C()const{}
};
)";
	COMPILE_PROGRAM(program, pa, input);

	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"Z", L"M"));

		AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::Y const * $PR"			);
		AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::Y $PR"					);
		AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::Y $PR"					);
		AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"double const $L"			);
		AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::Y $PR"					);
		AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::Y $PR"					);
	}
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"Z", L"C"));

		AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::X * $PR"				);
		AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::X $PR"					);
		AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::X $PR"					);
		AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"__int32 $L"				);
		AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::X $PR"					);
		AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::X $PR"					);
	}
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

auto&&			_A4 = &Y::A;
auto&&			_B4 = &Y::B;
auto&&			_C4 = &Y::C;
auto&&			_D4 = &Y::D;
auto&&			_E4 = &E;

const auto&		_A5 = &Y::A;
const auto&		_B5 = &Y::B;
const auto&		_C5 = &Y::C;
const auto&		_D5 = &Y::D;
const auto&		_E5 = &E;

auto&			_A6 = _A5;
auto&			_B6 = _B5;
auto&			_C6 = _C5;
auto&			_D6 = _D5;
auto&			_E6 = _E5;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"Y::A",			L"Y :: A",				L"__int32 $L"									);
	AssertExpr(pa, L"Y::B",			L"Y :: B",				L"__int32 $L"									);
	AssertExpr(pa, L"Y::C",			L"Y :: C",				L"__int32 __cdecl() * $PR"						);
	AssertExpr(pa, L"Y::D",			L"Y :: D",				L"__int32 __thiscall() * $PR"					);
	AssertExpr(pa, L"E",			L"E",					L"__int32 __cdecl() * $PR"						);

	AssertExpr(pa, L"&Y::A",		L"(& Y :: A)",			L"__int32 * $PR"								);
	AssertExpr(pa, L"&Y::B",		L"(& Y :: B)",			L"__int32 (::X ::) * $PR"						);
	AssertExpr(pa, L"&Y::C",		L"(& Y :: C)",			L"__int32 __cdecl() * $PR"						);
	AssertExpr(pa, L"&Y::D",		L"(& Y :: D)",			L"__int32 __thiscall() (::X ::) * $PR"			);
	AssertExpr(pa, L"&E",			L"(& E)",				L"__int32 __cdecl() * $PR"						);

	AssertExpr(pa, L"_A1",			L"_A1",					L"__int32 * $L"									);
	AssertExpr(pa, L"_B1",			L"_B1",					L"__int32 (::X ::) * $L"						);
	AssertExpr(pa, L"_C1",			L"_C1",					L"__int32 __cdecl() * $L"						);
	AssertExpr(pa, L"_D1",			L"_D1",					L"__int32 __thiscall() (::X ::) * $L"			);
	AssertExpr(pa, L"_E1",			L"_E1",					L"__int32 __cdecl() * $L"						);

	AssertExpr(pa, L"&_A1",			L"(& _A1)",				L"__int32 * * $PR"								);
	AssertExpr(pa, L"&_B1",			L"(& _B1)",				L"__int32 (::X ::) * * $PR"						);
	AssertExpr(pa, L"&_C1",			L"(& _C1)",				L"__int32 __cdecl() * * $PR"					);
	AssertExpr(pa, L"&_D1",			L"(& _D1)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
	AssertExpr(pa, L"&_E1",			L"(& _E1)",				L"__int32 __cdecl() * * $PR"					);

	AssertExpr(pa, L"_A2",			L"_A2",					L"__int32 * $L"									);
	AssertExpr(pa, L"_B2",			L"_B2",					L"__int32 (::X ::) * $L"						);
	AssertExpr(pa, L"_C2",			L"_C2",					L"__int32 __cdecl() * $L"						);
	AssertExpr(pa, L"_D2",			L"_D2",					L"__int32 __thiscall() (::X ::) * $L"			);
	AssertExpr(pa, L"_E2",			L"_E2",					L"__int32 __cdecl() * $L"						);

	AssertExpr(pa, L"&_A2",			L"(& _A2)",				L"__int32 * * $PR"								);
	AssertExpr(pa, L"&_B2",			L"(& _B2)",				L"__int32 (::X ::) * * $PR"						);
	AssertExpr(pa, L"&_C2",			L"(& _C2)",				L"__int32 __cdecl() * * $PR"					);
	AssertExpr(pa, L"&_D2",			L"(& _D2)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
	AssertExpr(pa, L"&_E2",			L"(& _E2)",				L"__int32 __cdecl() * * $PR"					);

	AssertExpr(pa, L"_A3",			L"_A3",					L"__int32 * [] $L"								);
	AssertExpr(pa, L"_B3",			L"_B3",					L"__int32 (::X ::) * [] $L"						);
	AssertExpr(pa, L"_C3",			L"_C3",					L"__int32 __cdecl() * [] $L"					);
	AssertExpr(pa, L"_D3",			L"_D3",					L"__int32 __thiscall() (::X ::) * [] $L"		);
	AssertExpr(pa, L"_E3",			L"_E3",					L"__int32 __cdecl() * [] $L"					);

	AssertExpr(pa, L"&_A3",			L"(& _A3)",				L"__int32 * [] * $PR"							);
	AssertExpr(pa, L"&_B3",			L"(& _B3)",				L"__int32 (::X ::) * [] * $PR"					);
	AssertExpr(pa, L"&_C3",			L"(& _C3)",				L"__int32 __cdecl() * [] * $PR"					);
	AssertExpr(pa, L"&_D3",			L"(& _D3)",				L"__int32 __thiscall() (::X ::) * [] * $PR"		);
	AssertExpr(pa, L"&_E3",			L"(& _E3)",				L"__int32 __cdecl() * [] * $PR"					);

	AssertExpr(pa, L"_A4",			L"_A4",					L"__int32 * && $L"								);
	AssertExpr(pa, L"_B4",			L"_B4",					L"__int32 (::X ::) * && $L"						);
	AssertExpr(pa, L"_C4",			L"_C4",					L"__int32 __cdecl() * && $L"					);
	AssertExpr(pa, L"_D4",			L"_D4",					L"__int32 __thiscall() (::X ::) * && $L"		);
	AssertExpr(pa, L"_E4",			L"_E4",					L"__int32 __cdecl() * && $L"					);

	AssertExpr(pa, L"&_A4",			L"(& _A4)",				L"__int32 * * $PR"								);
	AssertExpr(pa, L"&_B4",			L"(& _B4)",				L"__int32 (::X ::) * * $PR"						);
	AssertExpr(pa, L"&_C4",			L"(& _C4)",				L"__int32 __cdecl() * * $PR"					);
	AssertExpr(pa, L"&_D4",			L"(& _D4)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
	AssertExpr(pa, L"&_E4",			L"(& _E4)",				L"__int32 __cdecl() * * $PR"					);

	AssertExpr(pa, L"_A5",			L"_A5",					L"__int32 * const & $L"							);
	AssertExpr(pa, L"_B5",			L"_B5",					L"__int32 (::X ::) * const & $L"				);
	AssertExpr(pa, L"_C5",			L"_C5",					L"__int32 __cdecl() * const & $L"				);
	AssertExpr(pa, L"_D5",			L"_D5",					L"__int32 __thiscall() (::X ::) * const & $L"	);
	AssertExpr(pa, L"_E5",			L"_E5",					L"__int32 __cdecl() * const & $L"				);

	AssertExpr(pa, L"&_A5",			L"(& _A5)",				L"__int32 * const * $PR"						);
	AssertExpr(pa, L"&_B5",			L"(& _B5)",				L"__int32 (::X ::) * const * $PR"				);
	AssertExpr(pa, L"&_C5",			L"(& _C5)",				L"__int32 __cdecl() * const * $PR"				);
	AssertExpr(pa, L"&_D5",			L"(& _D5)",				L"__int32 __thiscall() (::X ::) * const * $PR"	);
	AssertExpr(pa, L"&_E5",			L"(& _E5)",				L"__int32 __cdecl() * const * $PR"				);

	AssertExpr(pa, L"_A6",			L"_A6",					L"__int32 * const & $L"							);
	AssertExpr(pa, L"_B6",			L"_B6",					L"__int32 (::X ::) * const & $L"				);
	AssertExpr(pa, L"_C6",			L"_C6",					L"__int32 __cdecl() * const & $L"				);
	AssertExpr(pa, L"_D6",			L"_D6",					L"__int32 __thiscall() (::X ::) * const & $L"	);
	AssertExpr(pa, L"_E6",			L"_E6",					L"__int32 __cdecl() * const & $L"				);

	AssertExpr(pa, L"&_A6",			L"(& _A6)",				L"__int32 * const * $PR"						);
	AssertExpr(pa, L"&_B6",			L"(& _B6)",				L"__int32 (::X ::) * const * $PR"				);
	AssertExpr(pa, L"&_C6",			L"(& _C6)",				L"__int32 __cdecl() * const * $PR"				);
	AssertExpr(pa, L"&_D6",			L"(& _D6)",				L"__int32 __thiscall() (::X ::) * const * $PR"	);
	AssertExpr(pa, L"&_E6",			L"(& _E6)",				L"__int32 __cdecl() * const * $PR"				);
}

TEST_CASE(TestParseExpr_AddressOfMember_OfExplicitOrImplicitThisExpr)
{
	auto input = LR"(
struct S
{
	int f;
	int& r;
	const int c;
	static int s;

	void M1(double p){}
	void C1(double p)const{}
	void V1(double p)volatile{}
	void CV1(double p)const volatile{}
	static void F1(double p){}

	void M2(double p);
	void C2(double p)const;
	void V2(double p)volatile;
	void CV2(double p)const volatile;
	static void F2(double p);
};

void S::M2(double p){}
void S::C2(double p)const{}
void S::V2(double p)volatile{}
void S::CV2(double p)const volatile{}
void S::F2(double p){}
)";
	COMPILE_PROGRAM(program, pa, input);
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"S", L"M" + itow(i)));

		AssertExpr(spa, L"this",					L"this",						L"::S * $PR"								);
		AssertExpr(spa, L"p",						L"p",							L"double $L"								);

		AssertExpr(spa, L"f",						L"f",							L"__int32 $L"								);
		AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 $L"								);
		AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
		AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
		AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
		AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
		AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
		AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
		AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
		AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
		AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
		AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
		AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
		AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

		AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
		AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
		AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"S", L"C" + itow(i)));

		AssertExpr(spa, L"this",					L"this",						L"::S const * $PR"							);
		AssertExpr(spa, L"p",						L"p",							L"double $L"								);

		AssertExpr(spa, L"f",						L"f",							L"__int32 const $L"							);
		AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 const $L"							);
		AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 const * $PR"						);
		AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
		AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 const $L"							);
		AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 const $L"							);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 const * $PR"						);
		AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 const * $PR"						);

		AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
		AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
		AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
		AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
		AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
		AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
		AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
		AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
		AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
		AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
		AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

		AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
		AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
		AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"S", L"V" + itow(i)));

		AssertExpr(spa, L"this",					L"this",						L"::S volatile * $PR"						);
		AssertExpr(spa, L"p",						L"p",							L"double $L"								);

		AssertExpr(spa, L"f",						L"f",							L"__int32 volatile $L"						);
		AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 volatile $L"						);
		AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 volatile * $PR"					);
		AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
		AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 volatile $L"						);
		AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 volatile $L"						);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 volatile * $PR"					);
		AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 volatile * $PR"					);

		AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
		AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
		AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
		AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
		AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"c",						L"c",							L"__int32 const volatile $L"				);
		AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
		AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const volatile * $PR"				);

		AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
		AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
		AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"S", L"CV" + itow(i)));

		AssertExpr(spa, L"this",					L"this",						L"::S const volatile * $PR"					);
		AssertExpr(spa, L"p",						L"p",							L"double $L"								);

		AssertExpr(spa, L"f",						L"f",							L"__int32 const volatile $L"				);
		AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
		AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 const volatile * $PR"				);

		AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
		AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
		AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
		AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
		AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"c",						L"c",							L"__int32 const volatile $L"				);
		AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
		AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const volatile $L"				);
		AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const volatile $L"				);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const volatile * $PR"				);
		AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const volatile * $PR"				);

		AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
		AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
		AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithContext(GetFunctionBodyStatementSymbol(pa, L"S", L"F" + itow(i)));

		AssertExpr(spa, L"this",					L"this",						L"::S * $PR"								);
		AssertExpr(spa, L"p",						L"p",							L"double $L"								);

		AssertExpr(spa, L"f",						L"f",							L"__int32 $L"								);
		AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 $L"								);
		AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
		AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
		AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
		AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
		AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
		AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

		AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
		AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
		AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
		AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
		AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
		AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
		AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

		AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
		AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
		AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
		AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
		AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
		AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
	}
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

	AssertExpr(pa, L"a1",			L"a1",				L"__int32 $L"						);
	AssertExpr(pa, L"b1",			L"b1",				L"__int32 $L"						);
	AssertExpr(pa, L"c1",			L"c1",				L"__int32 $L"						);
	AssertExpr(pa, L"a2",			L"a2",				L"__int32 $L"						);
	AssertExpr(pa, L"b2",			L"b2",				L"__int32 const $L"					);
	AssertExpr(pa, L"c2",			L"c2",				L"__int32 const $L"					);
	AssertExpr(pa, L"a3",			L"a3",				L"__int32 $L"						);
	AssertExpr(pa, L"b3",			L"b3",				L"__int32 const & $L"				);
	AssertExpr(pa, L"c3",			L"c3",				L"__int32 const & $L"				);
}

TEST_CASE(TestParseExpr_DeclType_Func)
{
	auto input = LR"(
struct X
{
	int x;
	auto a() { return a_(); }
	auto a_() { return (x); }
	decltype(auto) b() { return b_(); }
	decltype(auto) b_() { return (x); }
	auto c()->decltype(auto) { return c_(); }
	auto c_()->decltype(auto) { return (x); }
	auto r(int p) { if (!p) return 0; else return r(0); }
};
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"X().a()",			L"X().a()",				L"__int32 $PR"						);
	AssertExpr(pa, L"X().b()",			L"X().b()",				L"__int32 & $L"						);
	AssertExpr(pa, L"X().c()",			L"X().c()",				L"__int32 & $L"						);
	AssertExpr(pa, L"X().r(0)",			L"X().r(0)",			L"__int32 $PR"						);
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

	AssertExpr(pa, L"Spring",					L"Spring",					L"::Season $PR"							);
	AssertExpr(pa, L"Summer",					L"Summer",					L"::Season $PR"							);
	AssertExpr(pa, L"Autumn",					L"Autumn",					L"::Season $PR"							);
	AssertExpr(pa, L"Winter",					L"Winter",					L"::Season $PR"							);
	AssertExpr(pa, L"Season::Spring",			L"Season :: Spring",		L"::Season $PR"							);
	AssertExpr(pa, L"Season::Summer",			L"Season :: Summer",		L"::Season $PR"							);
	AssertExpr(pa, L"Season::Autumn",			L"Season :: Autumn",		L"::Season $PR"							);
	AssertExpr(pa, L"Season::Winter",			L"Season :: Winter",		L"::Season $PR"							);
	AssertExpr(pa, L"SeasonClass::Spring",		L"SeasonClass :: Spring",	L"::SeasonClass $PR"					);
	AssertExpr(pa, L"SeasonClass::Summer",		L"SeasonClass :: Summer",	L"::SeasonClass $PR"					);
	AssertExpr(pa, L"SeasonClass::Autumn",		L"SeasonClass :: Autumn",	L"::SeasonClass $PR"					);
	AssertExpr(pa, L"SeasonClass::Winter",		L"SeasonClass :: Winter",	L"::SeasonClass $PR"					);
}

TEST_CASE(TestParseExpr_FieldReference)
{
	return;
	auto input = LR"(
struct A
{
	int a;
	int A::* b;
	double C();
	double (A::*d)();
	double E(...)const;
	double (A::*f)(...)const;
};

A a;
A* pa;
const A ca;
volatile A* pva;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"a.a",						L"a.a",						L"__int32 $L"										);
	AssertExpr(pa, L"a.b",						L"a.b",						L"__int32 (::A ::) * $L"							);
	AssertExpr(pa, L"a.C",						L"a.C",						L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"a.d",						L"a.d",						L"double __thiscall() (::A ::) * $L"				);
	AssertExpr(pa, L"a.E",						L"a.E",						L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"a.f",						L"a.f",						L"double __cdecl(...) (::A ::) * $L"				);

	AssertExpr(pa, L"pa->a",					L"pa->a",					L"__int32 $L"										);
	AssertExpr(pa, L"pa->b",					L"pa->b",					L"__int32 (::A ::) * $L"							);
	AssertExpr(pa, L"pa->C",					L"pa->C",					L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"pa->d",					L"pa->d",					L"double __thiscall() (::A ::) * $L"				);
	AssertExpr(pa, L"pa->E",					L"pa->E",					L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"pa->f",					L"pa->f",					L"double __cdecl(...) (::A ::) * $L"				);

	AssertExpr(pa, L"ca.a",						L"ca.a",					L"__int32 const $L"									);
	AssertExpr(pa, L"ca.b",						L"ca.b",					L"__int32 (::A ::) * const $L"						);
	AssertExpr(pa, L"ca.C",						L"ca.C"																			);
	AssertExpr(pa, L"ca.d",						L"ca.d",					L"double __thiscall() (::A ::) * const $L"			);
	AssertExpr(pa, L"ca.E",						L"ca.E",					L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"ca.f",						L"ca.f",					L"double __cdecl(...) (::A ::) * const $L"			);

	AssertExpr(pa, L"pva->a",					L"pva->a",					L"__int32 volatile $L"								);
	AssertExpr(pa, L"pva->b",					L"pva->b",					L"__int32 (::A ::) * volatile $L"					);
	AssertExpr(pa, L"pva->C",					L"pva->C"																		);
	AssertExpr(pa, L"pva->d",					L"pva->d",					L"double __thiscall() (::A ::) * volatile $L"		);
	AssertExpr(pa, L"pva->E",					L"pva->E"																		);
	AssertExpr(pa, L"pva->f",					L"pva->f",					L"double __cdecl(...) (::A ::) * volatile $L"		);

	AssertExpr(pa, L"a.*&A::a",					L"(a .* (& A :: a))",		L"__int32 & $L"										);
	AssertExpr(pa, L"a.*&A::b",					L"(a .* (& A :: b))",		L"__int32 (::A ::) * & $L"							);
	AssertExpr(pa, L"a.*&A::C",					L"(a .* (& A :: C))",		L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"a.*&A::d",					L"(a .* (& A :: d))",		L"double __thiscall() (::A ::) * & $L"				);
	AssertExpr(pa, L"a.*&A::E",					L"(a .* (& A :: E))",		L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"a.*&A::f",					L"(a .* (& A :: f))",		L"double __cdecl(...) (::A ::) * & $L"				);

	AssertExpr(pa, L"pa->*&A::a",				L"(pa ->* (& A :: a))",		L"__int32 & $L"										);
	AssertExpr(pa, L"pa->*&A::b",				L"(pa ->* (& A :: b))",		L"__int32 (::A ::) * & $L"							);
	AssertExpr(pa, L"pa->*&A::C",				L"(pa ->* (& A :: C))",		L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"pa->*&A::d",				L"(pa ->* (& A :: d))",		L"double __thiscall() (::A ::) * & $L"				);
	AssertExpr(pa, L"pa->*&A::E",				L"(pa ->* (& A :: E))",		L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"pa->*&A::f",				L"(pa ->* (& A :: f))",		L"double __cdecl(...) (::A ::) * & $L"				);

	AssertExpr(pa, L"ca.*&A::a",				L"(ca .* (& A :: a))",		L"__int32 const & $L"								);
	AssertExpr(pa, L"ca.*&A::b",				L"(ca .* (& A :: b))",		L"__int32 (::A ::) * const & $L"					);
	AssertExpr(pa, L"ca.*&A::C",				L"(ca .* (& A :: C))",		L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"ca.*&A::d",				L"(ca .* (& A :: d))",		L"double __thiscall() (::A ::) * const & $L"		);
	AssertExpr(pa, L"ca.*&A::E",				L"(ca .* (& A :: E))",		L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"ca.*&A::f",				L"(ca .* (& A :: f))",		L"double __cdecl(...) (::A ::) * const & $L"		);

	AssertExpr(pa, L"pva->*&A::a",				L"(pva ->* (& A :: a))",	L"__int32 volatile & $L"							);
	AssertExpr(pa, L"pva->*&A::b",				L"(pva ->* (& A :: b))",	L"__int32 (::A ::) * volatile & $L"					);
	AssertExpr(pa, L"pva->*&A::C",				L"(pva ->* (& A :: C))",	L"double __thiscall() * $PR"						);
	AssertExpr(pa, L"pva->*&A::d",				L"(pva ->* (& A :: d))",	L"double __thiscall() (::A ::) * volatile & $L"		);
	AssertExpr(pa, L"pva->*&A::E",				L"(pva ->* (& A :: E))",	L"double __cdecl(...) * $PR"						);
	AssertExpr(pa, L"pva->*&A::f",				L"(pva ->* (& A :: f))",	L"double __cdecl(...) (::A ::) * volatile & $L"		);
}

TEST_CASE(TestParseExpr_Ternary_Comma)
{
	auto input = LR"(
struct A
{
	A(int);
	void F();
};
A a, b;
const A ca, cb;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"1?2:3",								L"(1 ? 2 : 3)",											L"__int32 $PR"				);
	AssertExpr(pa, L"1?2.0:3",								L"(1 ? 2.0 : 3)",										L"double $PR"				);
	AssertExpr(pa, L"1?2:3.0",								L"(1 ? 2 : 3.0)",										L"double $PR"				);

	AssertExpr(pa, L"1?\"abc\":nullptr",					L"(1 ? \"abc\" : nullptr)",								L"char const * $PR"			);
	AssertExpr(pa, L"1?nullptr:\"abc\"",					L"(1 ? nullptr : \"abc\")",								L"char const * $PR"			);
	AssertExpr(pa, L"1?\"abc\":0",							L"(1 ? \"abc\" : 0)",									L"char const * $PR"			);
	AssertExpr(pa, L"1?0:\"abc\"",							L"(1 ? 0 : \"abc\")",									L"char const * $PR"			);

	AssertExpr(pa, L"1?(const char*)\"abc\":nullptr",		L"(1 ? c_cast<char const *>(\"abc\") : nullptr)",		L"char const * $PR"			);
	AssertExpr(pa, L"1?nullptr:(const char*)\"abc\"",		L"(1 ? nullptr : c_cast<char const *>(\"abc\"))",		L"char const * $PR"			);
	AssertExpr(pa, L"1?(const char*)\"abc\":0",				L"(1 ? c_cast<char const *>(\"abc\") : 0)",				L"char const * $PR"			);
	AssertExpr(pa, L"1?0:(const char*)\"abc\"",				L"(1 ? 0 : c_cast<char const *>(\"abc\"))",				L"char const * $PR"			);

	AssertExpr(pa, L"1?A(0):0",								L"(1 ? A(0) : 0)",										L"::A $PR"					);
	AssertExpr(pa, L"1?0:A(0)",								L"(1 ? 0 : A(0))",										L"::A $PR"					);

	AssertExpr(pa, L"1?a:b",								L"(1 ? a : b)",											L"::A & $L"					);
	AssertExpr(pa, L"1?ca:b",								L"(1 ? ca : b)",										L"::A const & $L"			);
	AssertExpr(pa, L"1?a:cb",								L"(1 ? a : cb)",										L"::A const & $L"			);
	AssertExpr(pa, L"1?ca:cb",								L"(1 ? ca : cb)",										L"::A const & $L"			);

	AssertExpr(pa, L"1?(A&)a:(A&)b",						L"(1 ? c_cast<A &>(a) : c_cast<A &>(b))",				L"::A & $L"					);
	AssertExpr(pa, L"1?(const A&)ca:(A&)b",					L"(1 ? c_cast<A const &>(ca) : c_cast<A &>(b))",		L"::A const & $L"			);
	AssertExpr(pa, L"1?(A&)a:(const A&)cb",					L"(1 ? c_cast<A &>(a) : c_cast<A const &>(cb))",		L"::A const & $L"			);
	AssertExpr(pa, L"1?(const A&)ca:(const A&)cb",			L"(1 ? c_cast<A const &>(ca) : c_cast<A const &>(cb))",	L"::A const & $L"			);

	AssertExpr(pa, L"1?A():b",								L"(1 ? A() : b)",										L"::A $PR"					);
	AssertExpr(pa, L"1?a:A()",								L"(1 ? a : A())",										L"::A $PR"					);

	AssertExpr(pa, L"1?a.F():b.F()",						L"(1 ? a.F() : b.F())",									L"void $PR"					);
	AssertExpr(pa, L"1?0:A(0),true",						L"((1 ? 0 : A(0)) , true)",								L"bool $PR"					);
	AssertExpr(pa, L"true,1?0:A(0)",						L"(true , (1 ? 0 : A(0)))",								L"::A $PR"					);

	AssertExpr(pa, L"(1?a.F():b.F())",						L"((1 ? a.F() : b.F()))",								L"void $PR"					);
	AssertExpr(pa, L"(1?0:A(0),true)",						L"(((1 ? 0 : A(0)) , true))",							L"bool $PR"					);
	AssertExpr(pa, L"(true,1?0:A(0))",						L"((true , (1 ? 0 : A(0))))",							L"::A $PR"					);
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

	AssertExpr(pa, L"typeid(int)",							L"typeid(int)",											L"::std::type_info $L"			);
	AssertExpr(pa, L"typeid(0)",							L"typeid(0)",											L"::std::type_info $L"			);
	AssertExpr(pa, L"sizeof(int)",							L"sizeof(int)",											L"unsigned __int32 $PR"			);
	AssertExpr(pa, L"sizeof 0",								L"sizeof(0)",											L"unsigned __int32 $PR"			);
	AssertExpr(pa, L"throw 0",								L"throw(0)",											L"void $PR"						);

	AssertExpr(pa, L"new type_info",						L"new type_info",										L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new type_info(0)",						L"new type_info(0)",									L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new type_info{0}",						L"new type_info{0}",									L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new(0)type_info",						L"new (0) type_info",									L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new(0,1)type_info(0,1)",				L"new (0, 1) type_info(0, 1)",							L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new(0,1)type_info{0,1}",				L"new (0, 1) type_info{0, 1}",							L"::std::type_info * $PR"		);

	AssertExpr(pa, L"new type_info[10]",					L"new type_info [10]",									L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new (0)type_info[10]",					L"new (0) type_info [10]",								L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new (0,1)type_info[10,20]",			L"new (0, 1) type_info [(10 , 20)]",					L"::std::type_info * $PR"		);
	AssertExpr(pa, L"new (0,1)type_info[10][20][30]",		L"new (0, 1) type_info [30] [20] [10]",					L"::std::type_info [,] * $PR"	);

	AssertExpr(pa, L"delete 0",								L"delete (0)",											L"void $PR"						);
	AssertExpr(pa, L"delete [] 0",							L"delete[] (0)",										L"void $PR"						);
}

TEST_CASE(TestParseExpr_Universal_Initialization)
{
	auto input = LR"(
struct S{};
int a;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"{}",						L"{}",							L"{} $PR"													);
	AssertExpr(pa, L"{1}",						L"{1}",							L"{__int32 $PR} $PR"										);
	AssertExpr(pa, L"{1, 2}",					L"{1, 2}",						L"{__int32 $PR, __int32 $PR} $PR"							);
	AssertExpr(pa, L"{nullptr, (a), S()}",		L"{nullptr, (a), S()}",			L"{nullptr_t $PR, __int32 & $L, ::S $PR} $PR"				);
}

TEST_CASE(TestParseExpr_Lambda)
{
	// TODO
	// TsysType::Lambda
	// TsysType::CapturedLambda
	// GetElement() returns a function type
	// GetDecl() returns the scope inside the lambda
}