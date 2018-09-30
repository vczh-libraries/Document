#include <Ast_Type.h>
#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseType_Primitive)
{
#define TEST_PRIMITIVE_TYPE(TYPE)\
	AssertType(L#TYPE, L#TYPE);\
	AssertType(L"signed " L#TYPE, L"signed " L#TYPE);\
	AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE);\

	TEST_PRIMITIVE_TYPE(auto);
	TEST_PRIMITIVE_TYPE(void);
	TEST_PRIMITIVE_TYPE(bool);
	TEST_PRIMITIVE_TYPE(char);
	TEST_PRIMITIVE_TYPE(wchar_t);
	TEST_PRIMITIVE_TYPE(char16_t);
	TEST_PRIMITIVE_TYPE(char32_t);
	TEST_PRIMITIVE_TYPE(short);
	TEST_PRIMITIVE_TYPE(int);
	TEST_PRIMITIVE_TYPE(__int8);
	TEST_PRIMITIVE_TYPE(__int16);
	TEST_PRIMITIVE_TYPE(__int32);
	TEST_PRIMITIVE_TYPE(__int64);
	TEST_PRIMITIVE_TYPE(long);
	TEST_PRIMITIVE_TYPE(long long);
	TEST_PRIMITIVE_TYPE(float);
	TEST_PRIMITIVE_TYPE(double);
	TEST_PRIMITIVE_TYPE(long double);

#undef TEST_PRIMITIVE_TYPE
}

TEST_CASE(TestParseType_Short)
{
	AssertType(L"decltype(0)",						L"decltype(0)");
	AssertType(L"constexpr int",					L"int constexpr");
	AssertType(L"const int",						L"int const");
	AssertType(L"volatile int",						L"int volatile");
}

TEST_CASE(TestParseType_Long)
{
	AssertType(L"int constexpr",					L"int constexpr");
	AssertType(L"int const",						L"int const");
	AssertType(L"int volatile",						L"int volatile");
	AssertType(L"int ...",							L"int...");
	AssertType(L"int<long, short<float, double>>",	L"int<long, short<float, double>>");
}

TEST_CASE(TestParseType_ShortDeclarator)
{
	AssertType(L"int* __ptr32",						L"int *");
	AssertType(L"int* __ptr64",						L"int *");
	AssertType(L"int*",								L"int *");
	AssertType(L"int &",							L"int &");
	AssertType(L"int &&",							L"int &&");
	AssertType(L"int & &&",							L"int & &&");
}

TEST_CASE(TestParseType_LongDeclarator)
{
	AssertType(L"int[]",							L"int []");
	AssertType(L"int[][]",							L"int [] []");
	AssertType(L"int[1][2][3]",						L"int [1] [2] [3]");
	AssertType(L"int(*&)[][]",						L"int [] [] * &");

	AssertType(L"int()",																	L"int ()");
	AssertType(L"auto ()->int constexpr const volatile & && override noexcept throw()",		L"(auto->int constexpr const volatile & &&) () override noexcept throw()");
	AssertType(L"auto ()constexpr const volatile & && ->int override noexcept throw()",		L"(auto->int) () constexpr const volatile & && override noexcept throw()");

	AssertType(L"int __cdecl(int)",					L"int (int) __cdecl");
	AssertType(L"int __clrcall(int)",				L"int (int) __clrcall");
	AssertType(L"int __stdcall(int)",				L"int (int) __stdcall");
	AssertType(L"int __fastcall(int)",				L"int (int) __fastcall");
	AssertType(L"int __thiscall(int)",				L"int (int) __thiscall");
	AssertType(L"int __vectorcall(int)",			L"int (int) __vectorcall");
	AssertType(L"int(*)(int)",						L"int (int) *");
	AssertType(L"int(__cdecl*)(int)",				L"int (int) __cdecl *");

	AssertType(L"int(*[5])(int, int a, int b=0)",	L"int (int, a: int, b: int = 0) * [5]");
	AssertType(L"int(&(*[5])(void))[10]",			L"int [10] & () * [5]");
}

TEST_CASE(TestParseType_SuperComplexType)
{
	AssertType(
		L"int(__fastcall*const&((*)(int))[10])(int(&a)[], int(__stdcall*b)()noexcept, int(*c[5])(void)=0)",
		L"int (a: int [] &, b: int () noexcept __stdcall *, c: int () * [5] = 0) __fastcall * const & [10] (int) *"
		);
}

#define ASSERT_SYMBOL(INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)\
	if (name.nameTokens[0].rowStart == TROW && name.nameTokens[0].columnStart == TCOL)\
	{\
		TEST_ASSERT(name.name == NAME);\
		TEST_ASSERT(resolving->resolvedSymbols.Count() == 1);\
		auto decl = resolving->resolvedSymbols[0]->decls[0].Cast<TYPE>();\
		TEST_ASSERT(decl);\
		TEST_ASSERT(decl->name.name == NAME);\
		TEST_ASSERT(decl->name.nameTokens[0].rowStart == PROW);\
		TEST_ASSERT(decl->name.nameTokens[0].columnStart == PCOL);\
		if (!accessed.Contains(INDEX)) accessed.Add(INDEX);\
	} else \
	

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
			TEST_ASSERT(name.tokenCount > 0);
			ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
			ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
			ASSERT_SYMBOL(2, L"X", 0, 6, ForwardEnumDeclaration, 3, 6)
			TEST_ASSERT(false);
		});
		AssertType(
			L"a::b::X",
			L"a :: b :: X",
			pa);
		TEST_ASSERT(accessed.Count() == 3);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			TEST_ASSERT(name.tokenCount > 0);
			ASSERT_SYMBOL(0, L"a", 0, 11, NamespaceDeclaration, 1, 10)
			ASSERT_SYMBOL(1, L"b", 0, 14, NamespaceDeclaration, 1, 13)
			ASSERT_SYMBOL(2, L"X", 0, 17, ForwardEnumDeclaration, 3, 6)
			TEST_ASSERT(false);
		});
		AssertType(
			L"typename ::a::b::X::Y::Z",
			L"__root :: a :: typename b :: typename X :: typename Y :: typename Z",
			pa);
		TEST_ASSERT(accessed.Count() == 3);
	}
	{
		SortedList<vint> accessed;
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
			TEST_ASSERT(name.tokenCount > 0);
			ASSERT_SYMBOL(0, L"a", 0, 0, NamespaceDeclaration, 1, 10)
			ASSERT_SYMBOL(1, L"b", 0, 3, NamespaceDeclaration, 1, 13)
			ASSERT_SYMBOL(2, L"X", 0, 6, ForwardEnumDeclaration, 3, 6)
			ASSERT_SYMBOL(3, L"a", 0, 25, NamespaceDeclaration, 1, 10)
			ASSERT_SYMBOL(4, L"b", 0, 28, NamespaceDeclaration, 1, 13)
			TEST_ASSERT(false);
		});
		AssertType(
			L"a::b::X(__cdecl typename a::b::*)()",
			L"a :: b :: X () __cdecl (a :: typename b ::) *",
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
			TEST_ASSERT(name.tokenCount > 0);
			ASSERT_SYMBOL(0, L"c", 0, 0, NamespaceDeclaration, 8, 10)
			ASSERT_SYMBOL(1, L"d", 0, 3, NamespaceDeclaration, 8, 13)
			ASSERT_SYMBOL(2, L"Y", 0, 6, ClassDeclaration, 10, 8)
			ASSERT_SYMBOL(3, L"Z", 0, 9, ClassDeclaration, 12, 9)
			ASSERT_SYMBOL(4, L"Y", 0, 12, ForwardEnumDeclaration, 5, 7)
			TEST_ASSERT(false);
		});
		AssertType(
			L"c::d::Y::Z::Y",
			L"c :: d :: Y :: Z :: Y",
			pa);
		TEST_ASSERT(accessed.Count() == 5);
	}
}