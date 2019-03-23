#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericDecl_TypeAlias)
{
	auto input = LR"(
template<typename T>
using LRef = T&;

template<typename T>
using RRef = T&&;

template<typename T>
using Size = sizeof(T);

template<typename T>
using Ctor = T();

template<typename T, T Value>
using Id = Value;

template<typename, bool>
using True = true;
)";

	auto output = LR"(
template<typename T>
using LRef = T &;
template<typename T>
using RRef = T &&;
template<typename T>
using Size = sizeof(T);
template<typename T>
using Ctor = T();
template<typename T, T Value>
using Id = Value;
template<typename, bool>
using True = true;
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertType(L"LRef",				L"LRef",				L"<::LRef::typename[T]> ::LRef::typename[T] &",		pa);
	AssertType(L"RRef",				L"RRef",				L"<::RRef::typename[T]> ::RRef::typename[T] &&",	pa);
	AssertExpr(L"Size",				L"Size",				L"<::Size::typename[T]> unsigned __int32 $PR",		pa);
	AssertExpr(L"Ctor",				L"Ctor",				L"<::Ctor::typename[T]> ::Ctor::typename[T] $PR",	pa);
	AssertExpr(L"Id",				L"Id",					L"<::Id::typename[T], *> ::Id::typename[T] $PR",	pa);
	AssertExpr(L"True",				L"True",				L"<::True::typename[], *> bool $PR",				pa);
}

TEST_CASE(TestParseGenericDecl_TypeAlias_SimpleReplace)
{
	auto input = LR"(
template<typename T>
using Null = decltype(nullptr);

template<typename T>
using Int = int;

template<typename T>
using LRef = T&;

template<typename T>
using RRef = T&&;

template<typename T>
using Ptr = T*;

template<typename T>
using Array = T[10];

template<typename T, typename U>
using Function = T(*)(U);

template<typename T, typename U>
using Member = U T::*;

template<typename T>
using CV = const volatile T;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(L"Null<bool>",				L"Null<bool>",						L"nullptr_t",										pa);
	AssertType(L"Int<bool>",				L"Int<bool>",						L"__int32",											pa);
	AssertType(L"LRef<bool>",				L"LRef<bool>",						L"bool &",											pa);
	AssertType(L"RRef<bool>",				L"RRef<bool>",						L"bool &&",											pa);
	AssertType(L"Ptr<bool>",				L"Ptr<bool>",						L"bool *",											pa);
	AssertType(L"Array<bool>",				L"Array<bool>",						L"bool []",											pa);
	AssertType(L"Function<bool, char>",		L"Function<bool, char>",			L"bool __cdecl(char) *",							pa);
	AssertType(L"Member<bool, char>",		L"Member<bool, char>",				L"bool (char ::) *",								pa);
	AssertType(L"CV<bool>",					L"CV<bool>",						L"bool const volatile",								pa);
}