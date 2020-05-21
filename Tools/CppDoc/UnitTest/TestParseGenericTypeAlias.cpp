#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Normal")
	{
		auto input = LR"(
template<typename T>
using LRef = T&;

template<typename T>
using RRef = T&&;
)";

		auto output = LR"(
template<typename T>
using_type LRef: T &;
template<typename T>
using_type RRef: T &&;
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		AssertType(pa, L"LRef",				L"LRef",				L"<::LRef::[T]> ::LRef::[T] &"	);
		AssertType(pa, L"RRef",				L"RRef",				L"<::RRef::[T]> ::RRef::[T] &&"	);
	});

	TEST_CATEGORY(L"Simple replacing")
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

		AssertType(pa, L"Null<bool>",				L"Null<bool>",						L"nullptr_t"														);
		AssertType(pa, L"Int<bool>",				L"Int<bool>",						L"__int32"															);
		AssertType(pa, L"LRef<bool>",				L"LRef<bool>",						L"bool &"															);
		AssertType(pa, L"RRef<bool>",				L"RRef<bool>",						L"bool &&"															);
		AssertType(pa, L"Ptr<bool>",				L"Ptr<bool>",						L"bool *"															);
		AssertType(pa, L"Array<bool>",				L"Array<bool>",						L"bool []"															);
		AssertType(pa, L"Function<bool, char>",		L"Function<bool, char>",			L"bool __cdecl(char) *"												);
		AssertType(pa, L"Member<bool, char>",		L"Member<bool, char>",				L"bool (char ::) *"													);
		AssertType(pa, L"CV<bool>",					L"CV<bool>",						L"bool const volatile"												);
		AssertType(pa, L"ComplexType<S>",			L"ComplexType<S>",					L"nullptr_t [] && __cdecl(__int32 const volatile * &) * (::S ::) *"	);
	});

	TEST_CATEGORY(L"High level arguments")
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

		AssertType(pa, L"Container",				L"Container",						L"<::Container::[T], ::Container::[U]> any_t"						);
		AssertType(pa, L"Impl",						L"Impl",							L"<::Impl::[T], *> ::Impl::[T] __cdecl(::Impl::[T], __int32) *"		);
		AssertType(pa, L"Impl2",					L"Impl2",							L"<::Impl2::[T], *> ::Impl2::[T] __cdecl(::Impl2::[T], __int32) *"	);
		AssertType(pa, L"Container<double, Impl>",	L"Container<double, Impl>",			L"double __cdecl(double, __int32) *"								);
		AssertType(pa, L"Container<double, Impl2>",	L"Container<double, Impl2>",		L"double __cdecl(double, __int32) *"								);
	});

	TEST_CATEGORY(L"Keyword typename and template")
	{
		auto input = LR"(
struct S
{
	using X = int;

	template<typename T>
	using Y = T*;
};

template<typename T>
using X = typename T::X;

template<typename T>
using Y = typename T::template Y<T>;

template<template<typename> class T, typename U>
using Z = T<U>;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertType(pa, L"X",						L"X",								L"<::X::[T]> any_t"				);
		AssertType(pa, L"Y",						L"Y",								L"<::Y::[T]> any_t"				);
		AssertType(pa, L"Z",						L"Z",								L"<::Z::[T], ::Z::[U]> any_t"	);
		AssertType(pa, L"X<S>",						L"X<S>",							L"__int32"						);
		AssertType(pa, L"Y<S>",						L"Y<S>",							L"::S *"						);
		AssertType(pa, L"Z<S::Y, bool>",			L"Z<S :: Y, bool>",					L"bool *"						);
	});

	TEST_CATEGORY(L"Default value")
	{
		auto input = LR"(
char F(int, int);
float F(bool, bool);

template<typename T, typename U>
using Func = T(*)(U);

template<typename U, typename T>
using ReverseFunc = T(*)(U);

template<typename T, T TValue = T(), typename U = decltype(F(T(), TValue)), template<typename, typename> class V = Func>
using X = decltype({T{}, TValue, U{}, V<T, U>{}});
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertType(pa, L"X<int>",							L"X<int>",								L"{__int32 $PR, __int32 $PR, char $PR, __int32 __cdecl(char) * $PR}"		);
		AssertType(pa, L"X<bool>",							L"X<bool>",								L"{bool $PR, bool $PR, float $PR, bool __cdecl(float) * $PR}"				);
		AssertType(pa, L"X<int, {}>",						L"X<int, {}>",							L"{__int32 $PR, __int32 $PR, char $PR, __int32 __cdecl(char) * $PR}"		);
		AssertType(pa, L"X<bool, {}>",						L"X<bool, {}>",							L"{bool $PR, bool $PR, float $PR, bool __cdecl(float) * $PR}"				);
		AssertType(pa, L"X<int, {}, void*>",				L"X<int, {}, void *>",					L"{__int32 $PR, __int32 $PR, void * $PR, __int32 __cdecl(void *) * $PR}"	);
		AssertType(pa, L"X<bool, {}, void*>",				L"X<bool, {}, void *>",					L"{bool $PR, bool $PR, void * $PR, bool __cdecl(void *) * $PR}"				);
		AssertType(pa, L"X<int, {}, void*, ReverseFunc>",	L"X<int, {}, void *, ReverseFunc>",		L"{__int32 $PR, __int32 $PR, void * $PR, void * __cdecl(__int32) * $PR}"	);
		AssertType(pa, L"X<bool, {}, void*, ReverseFunc>",	L"X<bool, {}, void *, ReverseFunc>",	L"{bool $PR, bool $PR, void * $PR, void * __cdecl(bool) * $PR}"				);
	});
}