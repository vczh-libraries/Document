#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Literals")
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
	});

	TEST_CATEGORY(L"Names")
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
			TEST_CASE_ASSERT(accessed.Count() == 1);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"z2", 0, 3, ForwardFunctionDeclaration, 31, 4)
			END_ASSERT_SYMBOL;

			AssertExpr(pa, L" ::z2",		L"__root :: z2",			L"__int32 __cdecl(::c::Z) * $PR"						);
			AssertExpr(pa, L"&::z2",		L"(& __root :: z2)",		L"__int32 __cdecl(::c::Z) * $PR"						);
			TEST_CASE_ASSERT(accessed.Count() == 1);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 1, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"u1", 0, 4, ForwardVariableDeclaration, 9, 11)
			END_ASSERT_SYMBOL;

			AssertExpr(pa, L" Z::u1",		L"Z :: u1",					L"::a::X::Y $L"											);
			AssertExpr(pa, L"&Z::u1",		L"(& Z :: u1)",				L"::a::X::Y * $PR"										);
			TEST_CASE_ASSERT(accessed.Count() == 2);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 3, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"u2", 0, 6, VariableDeclaration, 10, 4)
			END_ASSERT_SYMBOL;

			AssertExpr(pa, L" ::Z::u2",		L"__root :: Z :: u2",		L"::a::X::Y $L"											);
			AssertExpr(pa, L"&::Z::u2",		L"(& __root :: Z :: u2)",	L"::a::X::Y (::a::X ::) * $PR"							);
			TEST_CASE_ASSERT(accessed.Count() == 2);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 1, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"v1", 0, 4, FunctionDeclaration, 11, 13)
			END_ASSERT_SYMBOL;

			AssertExpr(pa, L" Z::v1",		L"Z :: v1",					L"__int32 __cdecl(::a::X::Y &) * $PR"					);
			AssertExpr(pa, L"&Z::v1",		L"(& Z :: v1)",				L"__int32 __cdecl(::a::X::Y &) * $PR"					);
			TEST_CASE_ASSERT(accessed.Count() == 2);
		}
		{
			SortedList<vint> accessed;
			pa.recorder = BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"Z", 0, 3, ClassDeclaration, 23, 8)
				ASSERT_SYMBOL(1, L"v2", 0, 6, FunctionDeclaration, 12, 6)
			END_ASSERT_SYMBOL;

			AssertExpr(pa, L" ::Z::v2",		L"__root :: Z :: v2",		L"__int32 __thiscall(::a::X::Y &) * $PR"				);
			AssertExpr(pa, L"&::Z::v2",		L"(& __root :: Z :: v2)",	L"__int32 __thiscall(::a::X::Y &) (::a::X ::) * $PR"	);
			TEST_CASE_ASSERT(accessed.Count() == 2);
		}
	});

	TEST_CATEGORY(L"decltype + variables")
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
	});

	TEST_CATEGORY(L"decltype + arrays")
	{
		auto input = LR"(
int				x[10];
auto			a1 = x;
auto&			a2 = x;
const auto		a3 = x;
const auto&		a4 = x;

int				y[10][10];
auto			b1 = y;
auto&			b2 = y;
const auto		b3 = y;
const auto&		b4 = y;

auto			c1 = "text";
auto&			c2 = "text";
const auto		c3 = "text";
const auto&		c4 = "text";
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"a1",			L"a1",				L"__int32 * $L"						);
		AssertExpr(pa, L"a2",			L"a2",				L"__int32 [] & $L"					);
		AssertExpr(pa, L"a3",			L"a3",				L"__int32 * const $L"				);
		AssertExpr(pa, L"a4",			L"a4",				L"__int32 const [] & $L"			);

		AssertExpr(pa, L"b1",			L"b1",				L"__int32 [] * $L"					);
		AssertExpr(pa, L"b2",			L"b2",				L"__int32 [,] & $L"					);
		AssertExpr(pa, L"b3",			L"b3",				L"__int32 [] * const $L"			);
		AssertExpr(pa, L"b4",			L"b4",				L"__int32 const [,] & $L"			);

		AssertExpr(pa, L"c1",			L"c1",				L"char const * $L"					);
		AssertExpr(pa, L"c2",			L"c2",				L"char const [] & $L"				);
		AssertExpr(pa, L"c3",			L"c3",				L"char const * const $L"			);
		AssertExpr(pa, L"c4",			L"c4",				L"char const [] & $L"				);
	});

	TEST_CATEGORY(L"decltype + functions")
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
	});

	TEST_CATEGORY(L"enums")
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
	});

	TEST_CATEGORY(L"Commas")
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
	});

	TEST_CATEGORY(L"MISC")
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
	});

	TEST_CATEGORY(L"Universal initializations")
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
	});

	TEST_CATEGORY(L"Lambdas")
	{
		// TODO:
		// Generate TsysType::Struct for lambda
	});
}