#include <Ast_Expr.h>
#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseExpr_Literal)
{
	AssertExpr(L"true",			L"true",		L"bool"					);
	AssertExpr(L"false",		L"false",		L"bool"					);
	AssertExpr(L"nullptr",		L"nullptr",		L"nullptr_t"			);

	AssertExpr(L"1",			L"1",			L"__int32"				);
	AssertExpr(L"1u",			L"1u",			L"unsigned __int32"		);
	AssertExpr(L"1U",			L"1U",			L"unsigned __int32"		);
	AssertExpr(L"1l",			L"1l",			L"__int64"				);
	AssertExpr(L"1L",			L"1L",			L"__int64"				);
	AssertExpr(L"1ul",			L"1ul",			L"unsigned __int64"		);
	AssertExpr(L"1uL",			L"1uL",			L"unsigned __int64"		);
	AssertExpr(L"1Ul",			L"1Ul",			L"unsigned __int64"		);
	AssertExpr(L"1UL",			L"1UL",			L"unsigned __int64"		);

	AssertExpr(L"0x1",			L"0x1",			L"__int32"				);
	AssertExpr(L"0x1u",			L"0x1u",		L"unsigned __int32"		);
	AssertExpr(L"0x1U",			L"0x1U",		L"unsigned __int32"		);
	AssertExpr(L"0x1l",			L"0x1l",		L"__int64"				);
	AssertExpr(L"0x1L",			L"0x1L",		L"__int64"				);
	AssertExpr(L"0X1ul",		L"0X1ul",		L"unsigned __int64"		);
	AssertExpr(L"0X1uL",		L"0X1uL",		L"unsigned __int64"		);
	AssertExpr(L"0X1Ul",		L"0X1Ul",		L"unsigned __int64"		);
	AssertExpr(L"0X1UL",		L"0X1UL",		L"unsigned __int64"		);

	AssertExpr(L"0b1",			L"0b1",			L"__int32"				);
	AssertExpr(L"0b1u",			L"0b1u",		L"unsigned __int32"		);
	AssertExpr(L"0b1U",			L"0b1U",		L"unsigned __int32"		);
	AssertExpr(L"0b1l",			L"0b1l",		L"__int64"				);
	AssertExpr(L"0b1L",			L"0b1L",		L"__int64"				);
	AssertExpr(L"0B1ul",		L"0B1ul",		L"unsigned __int64"		);
	AssertExpr(L"0B1uL",		L"0B1uL",		L"unsigned __int64"		);
	AssertExpr(L"0B1Ul",		L"0B1Ul",		L"unsigned __int64"		);
	AssertExpr(L"0B1UL",		L"0B1UL",		L"unsigned __int64"		);
	
	AssertExpr(L"1.0",			L"1.0",			L"double"				);
	AssertExpr(L"1.0f",			L"1.0f",		L"float"				);
	AssertExpr(L"1.0F",			L"1.0F",		L"float"				);
	AssertExpr(L"1.0l",			L"1.0l",		L"double"				);
	AssertExpr(L"1.0L",			L"1.0L",		L"double"				);

	AssertExpr(L"'x'",			L"'x'",			L"char"					);
	AssertExpr(L"L'x'",			L"L'x'",		L"wchar_t"				);
	AssertExpr(L"u'x'",			L"u'x'",		L"char16_t"				);
	AssertExpr(L"U'x'",			L"U'x'",		L"char32_t"				);
	AssertExpr(L"u8'x'",		L"u8'x'",		L"char"					);

	AssertExpr(L"\"x\"",		L"\"x\"",		L"char const []"		);
	AssertExpr(L"L\"x\"",		L"L\"x\"",		L"wchar_t const []"		);
	AssertExpr(L"u\"x\"",		L"u\"x\"",		L"char16_t const []"	);
	AssertExpr(L"U\"x\"",		L"U\"x\"",		L"char32_t const []"	);
	AssertExpr(L"u8\"x\"",		L"u8\"x\"",		L"char const []"		);
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
		AssertExpr(L"z1",			L"z1",					L"::c::Z &",								pa);
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
		AssertExpr(L"::z2",			L"__root :: z2",		L"__int32 (::c::Z) *",						pa);
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
		AssertExpr(L"Z::u1",		L"Z :: u1",				L"::a::X::Y &",								pa);
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
		AssertExpr(L"::Z::u2",		L"__root :: Z :: u2",	L"::a::X::Y (::a::X ::)",					pa);
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
		AssertExpr(L"Z::v1",		L"Z :: v1",				L"__int32 (::a::X::Y &) *",					pa);
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
		AssertExpr(L"::Z::v2",		L"__root :: Z :: v2",	L"__int32 (::a::X::Y &) (::a::X ::) *",		pa);
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

	Y* operator->();
	X operator()(int);
	Y operator()(void*);
	X operator[](const char*);
	Y operator[](Z&&);

	static int F();
	int G();
};

Z z;
Z* pz = nullptr;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"pz->x",					L"pz->x",						L"bool &",				pa);
	AssertExpr(L"pz->y",					L"pz->y",						L"bool &",				pa);
	AssertExpr(L"pz->F",					L"pz->F",						L"int () *",			pa);
	AssertExpr(L"pz->G",					L"pz->G",						L"int () *",			pa);
	AssertExpr(L"pz->operator->",			L"pz->operator ->",				L"Y * () *",			pa);
	
	AssertExpr(L"pz->operator->()",			L"pz->operator ->()",			L"Y *",					pa);
	AssertExpr(L"pz->operator()(0)",		L"pz->operator ()(0)",			L"X",					pa);
	AssertExpr(L"pz->operator()(nullptr)",	L"pz->operator ()(nullptr)",	L"Y",					pa);
	AssertExpr(L"pz->operator[](\"a\")",	L"pz->operator [](\"a\")",		L"X",					pa);
	AssertExpr(L"pz->operator[](Z())",		L"pz->operator [](Z())",		L"Y",					pa);
	AssertExpr(L"pz->F(0)",					L"pz->F(0)",					L"int",					pa);
	AssertExpr(L"pz->G(0)",					L"pz->G(0)",					L"int",					pa);

	AssertExpr(L"z.x",						L"z.x",							L"bool &",				pa);
	AssertExpr(L"z.y",						L"z.y",							L"bool &",				pa);
	AssertExpr(L"z.F",						L"z.F",							L"int () *",			pa);
	AssertExpr(L"z.G",						L"z.G",							L"int () *",			pa);
	AssertExpr(L"z.operator->",				L"z.operator ->",				L"Y * () *",			pa);
	
	AssertExpr(L"z.operator->()",			L"z.operator ->()",				L"Y *",					pa);
	AssertExpr(L"z.operator()(0)",			L"z.operator ()(0)",			L"X",					pa);
	AssertExpr(L"z.operator()(nullptr)",	L"z.operator ()(nullptr)",		L"Y",					pa);
	AssertExpr(L"z.operator[](\"a\")",		L"z.operator [](\"a\")",		L"X",					pa);
	AssertExpr(L"z.operator[](Z())",		L"z.operator [](Z())",			L"Y",					pa);
	AssertExpr(L"z.F(0)",					L"z.F(0)",						L"int",					pa);
	AssertExpr(L"z.G(0)",					L"z.G(0)",						L"int",					pa);
	
	AssertExpr(L"z->x",						L"z->x",						L"int &",				pa);
	AssertExpr(L"z->y",						L"z->y",						L"int &",				pa);
	AssertExpr(L"z(0)",						L"z(0)",						L"X",					pa);
	AssertExpr(L"z(nullptr)",				L"z(nullptr)",					L"Y",					pa);
	AssertExpr(L"z[\"a\"]",					L"z[\"a\"]",					L"X",					pa);
	AssertExpr(L"z[Z()]",					L"z[Z()]",						L"Y",					pa);
}

TEST_CASE(TestParseExpr_ArrayAndPointerAccess)
{
}

TEST_CASE(TestParseExpr_OverloadingAndTypeConversion)
{
}