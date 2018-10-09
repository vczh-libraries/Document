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
		static int v1(Y&);
		int v2(Y&);
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
int Z2(Z);
)";
	COMPILE_PROGRAM(program, pa, input);
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"z1", 0, 0, VariableDeclaration, 30, 2)
				ASSERT_SYMBOL(1, L"z2", 0, 3, VariableDeclaration, 31, 4)
				ASSERT_SYMBOL(0, L"Z", 0, 0, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"Z", 0, 3, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(2, L"u", 0, 6, VariableDeclaration, 8, 11)
				ASSERT_SYMBOL(3, L"u", 0, 9, VariableDeclaration, 9, 4)
				ASSERT_SYMBOL(4, L"v", 0, 6, FunctionDeclaration, 10, 13)
				ASSERT_SYMBOL(5, L"w", 0, 9, FunctionDeclaration, 11, 6)
			END_ASSERT_SYMBOL
		});
		AssertExpr(L"z1",			L"z1",			L"::c::Z",								pa);
		AssertExpr(L"::z2",			L"::z2",		L"int (::c::Z) *",						pa);
		AssertExpr(L"Z::u1",		L"z :: u1",		L"::a::X::Y &",							pa);
		AssertExpr(L"::Z::u2",		L":: z :: u2",	L"::a::X::Y (::a::X ::)",				pa);
		AssertExpr(L"Z::v1",		L"z :: v1",		L"int (::a::X::Y &) *",					pa);
		AssertExpr(L"::Z::v2",		L":: z :: v2",	L"int (::a::X::Y &) (::a::X ::) *",		pa);
		TEST_ASSERT(accessed.Count() == 4);
	}
}