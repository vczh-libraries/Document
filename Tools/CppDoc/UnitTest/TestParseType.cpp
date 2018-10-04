#include <Ast_Type.h>
#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseType_Primitive)
{
#define TEST_PRIMITIVE_TYPE(TYPE, LOG, LOGS, LOGU)\
	AssertType(L#TYPE, L#TYPE, LOG);\
	AssertType(L"signed " L#TYPE, L"signed " L#TYPE, LOGS);\
	AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE, LOGU);\

	TEST_PRIMITIVE_TYPE(auto,			L"",			L"",					L""					);
	TEST_PRIMITIVE_TYPE(void,			L"void",		L"",					L""					);
	TEST_PRIMITIVE_TYPE(bool,			L"bool",		L"",					L""					);
	TEST_PRIMITIVE_TYPE(char,			L"char",		L"char",				L"unsigned char"	);
	TEST_PRIMITIVE_TYPE(wchar_t,		L"wchar_t",		L"signed wchar_t",		L"wchar_t"			);
	TEST_PRIMITIVE_TYPE(char16_t,		L"char16_t",	L"signed char16_t",		L"char16_t"			);
	TEST_PRIMITIVE_TYPE(char32_t,		L"char32_t",	L"signed char32_t",		L"char32_t"			);
	TEST_PRIMITIVE_TYPE(short,			L"__int16",		L"__int16",				L"unsigned __int16"	);
	TEST_PRIMITIVE_TYPE(int,			L"__int32",		L"__int32",				L"unsigned __int32"	);
	TEST_PRIMITIVE_TYPE(__int8,			L"__int8",		L"__int8",				L"unsigned __int8"	);
	TEST_PRIMITIVE_TYPE(__int16,		L"__int16",		L"__int16",				L"unsigned __int16"	);
	TEST_PRIMITIVE_TYPE(__int32,		L"__int32",		L"__int32",				L"unsigned __int32"	);
	TEST_PRIMITIVE_TYPE(__int64,		L"__int64",		L"__int64",				L"unsigned __int64"	);
	TEST_PRIMITIVE_TYPE(long,			L"__int32",		L"__int32",				L"unsigned __int32"	);
	TEST_PRIMITIVE_TYPE(long int,		L"__int32",		L"__int32",				L"unsigned __int32"	);
	TEST_PRIMITIVE_TYPE(long long,		L"__int64",		L"__int64",				L"unsigned __int64"	);
	TEST_PRIMITIVE_TYPE(float,			L"float",		L"",					L""					);
	TEST_PRIMITIVE_TYPE(double,			L"double",		L"",					L""					);
	TEST_PRIMITIVE_TYPE(long double,	L"double",		L"",					L""					);

#undef TEST_PRIMITIVE_TYPE
}

TEST_CASE(TestParseType_Short)
{
	AssertType(L"decltype(auto)",					L"decltype(auto)",						L""									);
	AssertType(L"decltype(0)",						L"decltype(0)",							L""									);
	AssertType(L"constexpr int",					L"int constexpr",						L"__int32 constexpr"				);
	AssertType(L"const int",						L"int const",							L"__int32 const"					);
	AssertType(L"volatile int",						L"int volatile",						L"__int32 volatile"					);
	AssertType(L"constexpr const int",				L"int constexpr const",					L"__int32 constexpr const"			);
	AssertType(L"const volatile int",				L"int const volatile",					L"__int32 const volatile"			);
	AssertType(L"volatile constexpr int",			L"int constexpr volatile",				L"__int32 constexpr volatile"		);
	AssertType(L"volatile const constexpr int",		L"int constexpr const volatile",		L"__int32 constexpr const volatile"	);
}

TEST_CASE(TestParseType_Long)
{
	AssertType(L"int constexpr",					L"int constexpr",						L"__int32 constexpr"				);
	AssertType(L"int const",						L"int const",							L"__int32 const"					);
	AssertType(L"int volatile",						L"int volatile",						L"__int32 volatile"					);
	AssertType(L"int constexpr const",				L"int constexpr const",					L"__int32 constexpr const"			);
	AssertType(L"int const volatile",				L"int const volatile",					L"__int32 const volatile"			);
	AssertType(L"int volatile constexpr",			L"int constexpr volatile",				L"__int32 constexpr volatile"		);
	AssertType(L"int volatile const constexpr",		L"int constexpr const volatile",		L"__int32 constexpr const volatile"	);
	AssertType(L"int ...",							L"int...",								L"");
	AssertType(L"int<long, short<float, double>>",	L"int<long, short<float, double>>",		L"");
}

TEST_CASE(TestParseType_ShortDeclarator)
{
	AssertType(L"int* __ptr32",						L"int *",								L"__int32 *"		);
	AssertType(L"int* __ptr64",						L"int *",								L"__int32 *"		);
	AssertType(L"int*",								L"int *",								L"__int32 *"		);
	AssertType(L"int&",								L"int &",								L"__int32 &"		);
	AssertType(L"int&&",							L"int &&",								L"__int32 &&"		);
	AssertType(L"int& &&",							L"int & &&",							L"__int32 &"		);
	AssertType(L"int&& &",							L"int && &",							L"__int32 &"		);
}

TEST_CASE(TestParseType_LongDeclarator)
{
	AssertType(L"int[]",							L"int []",								L"__int32 []"		);
	AssertType(L"int[][]",							L"int [] []",							L"__int32 [,]"		);
	AssertType(L"int[1][2][3]",						L"int [3] [2] [1]",						L"__int32 [,,]"		);
	AssertType(L"int([1])[2][3]",					L"int [3] [2] [1]",						L"__int32 [,,]"		);
	AssertType(L"int(*&)[][]",						L"int [] [] * &",						L"__int32 [,] * &"	);

	AssertType(L"int()",																	L"int ()",																	L"__int32 ()"							);
	AssertType(L"auto ()->int constexpr const volatile & && override noexcept throw()",		L"(auto->int constexpr const volatile & &&) () override noexcept throw()",	L"__int32 constexpr const volatile & ()");
	AssertType(L"auto ()constexpr const volatile & && ->int override noexcept throw()",		L"(auto->int) () constexpr const volatile & && override noexcept throw()",	L"__int32 ()"							);

	AssertType(L"int __cdecl(int)",					L"int (int) __cdecl",					L"__int32 (__int32)"						);
	AssertType(L"int __clrcall(int)",				L"int (int) __clrcall",					L"__int32 (__int32)"						);
	AssertType(L"int __stdcall(int)",				L"int (int) __stdcall",					L"__int32 (__int32)"						);
	AssertType(L"int __fastcall(int)",				L"int (int) __fastcall",				L"__int32 (__int32)"						);
	AssertType(L"int __thiscall(int)",				L"int (int) __thiscall",				L"__int32 (__int32)"						);
	AssertType(L"int __vectorcall(int)",			L"int (int) __vectorcall",				L"__int32 (__int32)"						);
	AssertType(L"int(*)(int)",						L"int (int) *",							L"__int32 (__int32) *"						);
	AssertType(L"int(__cdecl*)(int)",				L"int (int) __cdecl *",					L"__int32 (__int32) *"						);

	AssertType(L"int(*[5])(int, int a, int b=0)",	L"int (int, a: int, b: int = 0) * [5]",	L"__int32 (__int32, __int32, __int32) * []"	);
	AssertType(L"int(&(*[5])(void))[10]",			L"int [10] & () * [5]",					L"__int32 [] & () * []"						);
}

TEST_CASE(TestParseType_SuperComplexType)
{
	AssertType(
		L"int(__fastcall*const&((*)(int))[10])(int(&a)[], int(__stdcall*b)()noexcept, int(*c[5])(void)=0)",
		L"int (a: int [] &, b: int () noexcept __stdcall *, c: int () * [5] = 0) __fastcall * const & [10] (int) *",
		L"__int32 (__int32 [] &, __int32 () *, __int32 () * []) * const & [] (__int32) *"
		);
}

TEST_CASE(TestParseType_MemberType)
{
	auto input = LR"(
namespace a::b
{
	enum X;
}
)";
	COMPILE_PROGRAM(program, pa, input);
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(2, L"X", 0, 6, ForwardEnumDeclaration, 3, 6)
			END_ASSERT_SYMBOL
		});
		AssertType(
			L"a::b::X",
			L"a :: b :: X",
			L"::a::b::X",
			pa);
		TEST_ASSERT(accessed.Count() == 3);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 11, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 14, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(2, L"X", 0, 17, ForwardEnumDeclaration, 3, 6)
			END_ASSERT_SYMBOL
		});
		AssertType(
			L"typename ::a::b::X::Y::Z",
			L"__root :: a :: typename b :: typename X :: typename Y :: typename Z",
			L"",
			pa);
		TEST_ASSERT(accessed.Count() == 3);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
				ASSERT_SYMBOL(2, L"X", 0, 6, ForwardEnumDeclaration, 3, 6)
				ASSERT_SYMBOL(3, L"a", 0, 25, NamespaceDeclaration, 1, 10)
				ASSERT_SYMBOL(4, L"b", 0, 28, NamespaceDeclaration, 1, 13)
			END_ASSERT_SYMBOL
		});
		AssertType(
			L"a::b::X(__cdecl typename a::b::*)()",
			L"a :: b :: X () __cdecl (a :: typename b ::) *",
			L"::a::b::X () (::a::b ::) *",
			pa);
		TEST_ASSERT(accessed.Count() == 5);
	}
}

TEST_CASE(TestParseType_MemberType2)
{
	auto input = LR"(
namespace a::b
{
	struct X
	{
		enum Y;
	};
}
namespace c::d
{
	struct Y
	{
		struct Z : a::b::X
		{
		};
	};
}
)";
	COMPILE_PROGRAM(program, pa, input);
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			BEGIN_ASSERT_SYMBOL
				ASSERT_SYMBOL(0, L"c", 0, 0, NamespaceDeclaration, 8, 10)
				ASSERT_SYMBOL(1, L"d", 0, 3, NamespaceDeclaration, 8, 13)
				ASSERT_SYMBOL(2, L"Y", 0, 6, ClassDeclaration, 10, 8)
				ASSERT_SYMBOL(3, L"Z", 0, 9, ClassDeclaration, 12, 9)
				ASSERT_SYMBOL(4, L"Y", 0, 12, ForwardEnumDeclaration, 5, 7)
			END_ASSERT_SYMBOL
		});
		AssertType(
			L"c::d::Y::Z::Y",
			L"c :: d :: Y :: Z :: Y",
			L"::a::b::X::Y",
			pa);
		TEST_ASSERT(accessed.Count() == 5);
	}
}