#include <Ast_Type.h>
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
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
		});
		AssertType(
			L"a::b::X",
			L"a :: b :: X",
			pa);
	}
	{
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
		});
		AssertType(
			L"::a::b::X :: typename Y :: typename Z",
			L"__root :: a :: b :: X :: typename Y :: typename Z",
			pa);
	}
	{
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
		});
		AssertType(
			L"a::b::X(__cdecl a::typename b::*)()",
			L"a :: b :: X () __cdecl (a :: typename b ::) *",
			pa);
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
		pa.recorder = CreateTestIndexRecorder([&](CppName& name, Ptr<Resolving> resolving)
		{
		});
		AssertType(
			L"c::d::Y::Z::Y",
			L"c :: d :: Y :: Z :: Y",
			pa);
	}
}