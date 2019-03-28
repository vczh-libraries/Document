#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericDecl_TypeAlias)
{
	auto input = LR"(
template<typename T>
using LRef = T&;

template<typename T>
using RRef = T&&;
)";

	auto output = LR"(
template<typename T>
using LRef = T &;
template<typename T>
using RRef = T &&;
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertType(L"LRef",				L"LRef",				L"<::LRef::[T]> ::LRef::[T] &",				pa);
	AssertType(L"RRef",				L"RRef",				L"<::RRef::[T]> ::RRef::[T] &&",			pa);
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
using Member = T U::*;

template<typename T>
using CV = const volatile T;

struct S;
template<typename T>
using ComplexType = Member<Function<RRef<Array<Null<char>>>, LRef<Ptr<CV<Int<bool>>>>>, T>;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(L"Null<bool>",				L"Null<bool>",						L"nullptr_t",																pa);
	AssertType(L"Int<bool>",				L"Int<bool>",						L"__int32",																	pa);
	AssertType(L"LRef<bool>",				L"LRef<bool>",						L"bool &",																	pa);
	AssertType(L"RRef<bool>",				L"RRef<bool>",						L"bool &&",																	pa);
	AssertType(L"Ptr<bool>",				L"Ptr<bool>",						L"bool *",																	pa);
	AssertType(L"Array<bool>",				L"Array<bool>",						L"bool []",																	pa);
	AssertType(L"Function<bool, char>",		L"Function<bool, char>",			L"bool __cdecl(char) *",													pa);
	AssertType(L"Member<bool, char>",		L"Member<bool, char>",				L"bool (char ::) *",														pa);
	AssertType(L"CV<bool>",					L"CV<bool>",						L"bool const volatile",														pa);
	AssertType(L"ComplexType<S>",			L"ComplexType<S>",					L"nullptr_t [] && __cdecl(__int32 const volatile * &) * (::S ::) *",		pa);
}

TEST_CASE(TestParseGenericDecl_TypeAlias_HighLevelArgument)
{
	auto input = LR"(
template<typename T, template<typename U, U> class U>
using Container = U<T, {}>;

template<typename T, T Value>
using Impl = T(*)(T, int);

template<typename T, T Value>
using Impl2 = T(*)(decltype(Value), int);
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(L"Container",				L"Container",						L"<::Container::[T], <::Container::[U]::[U], *> any_t> any_t",			pa);
	AssertType(L"Impl",						L"Impl",							L"<::Impl::[T], *> ::Impl::[T] __cdecl(::Impl::[T], __int32) *",		pa);
	AssertType(L"Impl2",					L"Impl2",							L"<::Impl2::[T], *> ::Impl2::[T] __cdecl(::Impl2::[T], __int32) *",		pa);
	AssertType(L"Container<double, Impl>",	L"Container<double, Impl>",			L"double __cdecl(double, __int32) *",									pa);
	AssertType(L"Container<double, Impl2>",	L"Container<double, Impl2>",		L"double __cdecl(double, __int32) *",									pa);
}

TEST_CASE(TestParseGenericDecl_ValueAlias)
{
	auto input = LR"(
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

	AssertExpr(L"Size",				L"Size",				L"<::Size::[T]> unsigned __int32 $PR",		pa);
	AssertExpr(L"Ctor",				L"Ctor",				L"<::Ctor::[T]> ::Ctor::[T] $PR",			pa);
	AssertExpr(L"Id",				L"Id",					L"<::Id::[T], *> ::Id::[T] $PR",			pa);
	AssertExpr(L"True",				L"True",				L"<::True::[], *> bool $PR",				pa);
}

TEST_CASE(TestParseGenericDecl_ValueAlias_GenericExpr)
{
	auto input = LR"(
template<typename T, T Value>
using Id = Value;

template<typename, bool>
using True = true;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"Id<int, 0>",				L"Id<int, 0>",						L"__int32 $PR",				pa);
	AssertExpr(L"True<bool, true>",			L"True<bool, true>",				L"bool $PR",				pa);
}