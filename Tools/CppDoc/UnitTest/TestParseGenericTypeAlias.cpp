#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseTypeAlias)
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

	AssertType(pa, L"LRef",				L"LRef",				L"<::LRef::[T]> ::LRef::[T] &"	);
	AssertType(pa, L"RRef",				L"RRef",				L"<::RRef::[T]> ::RRef::[T] &&"	);
}

TEST_CASE(TestParseTypeAlias_SimpleReplace)
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
}

TEST_CASE(TestParseTypeAlias_HighLevelArgument)
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

	AssertType(pa, L"Container",				L"Container",						L"<::Container::[T], <::Container::[U]::[U], *> any_t> any_t"		);
	AssertType(pa, L"Impl",						L"Impl",							L"<::Impl::[T], *> ::Impl::[T] __cdecl(::Impl::[T], __int32) *"		);
	AssertType(pa, L"Impl2",					L"Impl2",							L"<::Impl2::[T], *> ::Impl2::[T] __cdecl(::Impl2::[T], __int32) *"	);
	AssertType(pa, L"Container<double, Impl>",	L"Container<double, Impl>",			L"double __cdecl(double, __int32) *"								);
	AssertType(pa, L"Container<double, Impl2>",	L"Container<double, Impl2>",		L"double __cdecl(double, __int32) *"								);
}

TEST_CASE(TestParseTypeAlias_TypenameTemplate)
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

	AssertType(pa, L"X",						L"X",								L"<::X::[T]> any_t"							);
	AssertType(pa, L"Y",						L"Y",								L"<::Y::[T]> any_t"							);
	AssertType(pa, L"Z",						L"Z",								L"<<::Z::[T]::[]> any_t, ::Z::[U]> any_t"	);
	AssertType(pa, L"X<S>",						L"X<S>",							L"__int32"									);
	AssertType(pa, L"Y<S>",						L"Y<S>",							L"::S *"									);
	AssertType(pa, L"Z<S::Y, bool>",			L"Z<S :: Y, bool>",					L"bool *"									);
}

TEST_CASE(TestParseTypeAlias_DefaultValue)
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
}

TEST_CASE(TestParseTypeAlias_VTA_Apply)
{
	auto input = LR"(

template<typename... TTypes>
using SizeOfTypes = decltype(sizeof...(TTypes));

template<int(*... Numbers)()>
using SizeOfExprs = decltype(sizeof...Numbers);
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"SizeOfTypes",									L"SizeOfTypes",									L"<...::SizeOfTypes::[TTypes]> unsigned __int32"			);
	AssertType(pa, L"SizeOfExprs",									L"SizeOfExprs",									L"<...*> unsigned __int32"									);
	
	AssertType(pa, L"SizeOfTypes<>",								L"SizeOfTypes<>",								L"unsigned __int32"											);
	AssertType(pa, L"SizeOfTypes<int>",								L"SizeOfTypes<int>",							L"unsigned __int32"											);
	AssertType(pa, L"SizeOfTypes<int, bool, char, double>",			L"SizeOfTypes<int, bool, char, double>",		L"unsigned __int32"											);
	AssertType(pa, L"SizeOfExprs<>",								L"SizeOfExprs<>",								L"unsigned __int32"											);
	AssertType(pa, L"SizeOfExprs<0>",								L"SizeOfExprs<0>",								L"unsigned __int32"											);
	AssertType(pa, L"SizeOfExprs<0, 1, 2, 3>",						L"SizeOfExprs<0, 1, 2, 3>",						L"unsigned __int32"											);
}

TEST_CASE(TestParseTypeAlias_VTA_Types)
{
	auto input = LR"(
template<typename T>
using Ptr = T*;

template<typename R, typename... TArgs>
using Func = R(*)(Ptr<TArgs>&&...);
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"Func",											L"Func",										L"<::Func::[R], ::Func::[...TArgs]> any_t"					);

	AssertType(pa, L"Func<bool>",									L"Func<bool>",									L"bool () *"												);
	AssertType(pa, L"Func<bool, int>",								L"Func<bool, int>",								L"bool (int * &&) *"										);
	AssertType(pa, L"Func<bool, int, bool, char, double>",			L"Func<bool, int, bool, char, double>",			L"bool (int * &&, bool * &&, char * &&, double * &&) *"		);
}

TEST_CASE(TestParseTypeAlias_VTA_Exprs)
{
	auto input = LR"(
template<typename T>
using Ptr = T*;

template<typename R, typename... TArgs>
using Init = R(*)(decltype({Ptr<TArgs>*{}...}));

char F(char, bool);
bool F(bool, char);
int F(int, float, double);
int* F(int*, float*, double*);
double F();
void F(...);

template<typename... TArgs>
using FOf = decltype(F(TArgs()...););
)";
	COMPILE_PROGRAM(program, pa, input);

#define TYPE_LIST_STRING(A, B, C, D, E, F)		\
	L"<::FOf::[...TArgs]> " L ## #A,			\
	L"<::FOf::[...TArgs]> " L ## #B,			\
	L"<::FOf::[...TArgs]> " L ## #C,			\
	L"<::FOf::[...TArgs]> " L ## #D,			\
	L"<::FOf::[...TArgs]> " L ## #E,			\
	L"<::FOf::[...TArgs]> " L ## #F				\

	
	AssertType(pa, L"Init",											L"Init",										L"<::Init::[R], ::Init::[...TArgs]> any_t"									);
	AssertType(pa, L"FOf",											L"FOf",											TYPE_LIST_STRING(char, bool, int, int *, double, void)						);

	AssertType(pa, L"Init<bool>",									L"Init<bool>",									L"bool ({}) *"																);
	AssertType(pa, L"Init<bool, int>",								L"Init<bool, int>",								L"bool ({int * * $PR}) *"													);
	AssertType(pa, L"Init<bool, int, bool, char, double>",			L"Init<bool, int, bool, char, double>",			L"bool ({int * * $PR, bool * * $PR, char * * $PR, double * * $PR}) *"		);
	AssertType(pa, L"FOf<>",										L"FOf<>",										L"double"																	);
	AssertType(pa, L"FOf<int>",										L"FOf<int>",									L"void"																		);
	AssertType(pa, L"FOf<char, bool>",								L"FOf<char, bool>",								L"char"																		);
	AssertType(pa, L"FOf<bool, char>",								L"FOf<bool, char>",								L"bool"																		);
	AssertType(pa, L"FOf<int, float, double>",						L"FOf<int, float, double>",						L"int"																		);
	AssertType(pa, L"FOf<int*, float*, double*>",					L"FOf<int *, float *, double *>",				L"int *"																	);

#undef TYPE_LIST_STRING
}

TEST_CASE(TestParseTypeAlias_VTA_ApplyOn_VTA_Default)
{
	auto input = LR"(
template<typename T = int*>											using One =				decltype({T{}});
template<typename T = int*, typename U = char*>						using Two =				decltype({T{}, U{}});
template<typename... TArgs>											using Vta =				decltype({TArgs{}...});
template<typename T = int*, typename... TArgs>						using OneVta =			decltype({T{}, TArgs{}...});
template<typename T = int*, typename U = char*, typename... TArgs>	using TwoVta =			decltype({T{}, U{}, TArgs{}...});

template<typename... Ts>											using ApplyOne =		One<Ts...>;
template<typename... Ts>											using ApplyTwo =		Two<Ts...>;
template<typename... Ts>											using ApplyVta =		Vta<Ts...>;
template<typename... Ts>											using ApplyOneVta =		OneVta<Ts...>;
template<typename... Ts>											using ApplyTwoVta =		TwoVta<Ts...>;

template<typename T, typename... Ts>								using ApplyOne_1 =		One<T, Ts...>;
template<typename T, typename... Ts>								using ApplyTwo_1 =		Two<T, Ts...>;
template<typename T, typename... Ts>								using ApplyVta_1 =		Vta<T, Ts...>;
template<typename T, typename... Ts>								using ApplyOneVta_1 =	OneVta<T, Ts...>;
template<typename T, typename... Ts>								using ApplyTwoVta_1 =	TwoVta<T, Ts...>;
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"ApplyOne",									L"ApplyOne",									L"<::ApplyOne::[...Ts]> {any_t $PR}"											);
	AssertType(pa, L"ApplyTwo",									L"ApplyTwo",									L"<::ApplyTwo::[...Ts]> {any_t $PR, any_t $PR}"									);
	AssertType(pa, L"ApplyVta",									L"ApplyVta",									L"<::ApplyVta::[...Ts]> any_t"													);
	AssertType(pa, L"ApplyOneVta",								L"ApplyOneVta",									L"<::ApplyOneVta::[...Ts]> any_t"												);
	AssertType(pa, L"ApplyTwoVta",								L"ApplyTwoVta",									L"<::ApplyTwoVta::[...Ts]> any_t"												);
	
	AssertType(pa, L"ApplyOne_1",								L"ApplyOne_1",									L"<::ApplyOne_1::[T], ::ApplyOne_1::[...Ts]> {::ApplyOne_1::[T] $PR}"			);
	AssertType(pa, L"ApplyTwo_1",								L"ApplyTwo_1",									L"<::ApplyTwo_1::[T], ::ApplyTwo_1::[...Ts]> {::ApplyTwo_1::[T] $PR, any_t $PR}");
	AssertType(pa, L"ApplyVta_1",								L"ApplyVta_1",									L"<::ApplyVta_1::[T], ::ApplyVta_1::[...Ts]> any_t"								);
	AssertType(pa, L"ApplyOneVta_1",							L"ApplyOneVta_1",								L"<::ApplyOneVta_1::[T], ::ApplyOneVta_1::[...Ts]> any_t"						);
	AssertType(pa, L"ApplyTwoVta_1",							L"ApplyTwoVta_1",								L"<::ApplyTwoVta_1::[T], ::ApplyTwoVta_1::[...Ts]> any_t"						);
	
	AssertType(pa, L"ApplyOne<>",								L"ApplyOne<>",									L"{int * $PR}"																	);
	AssertType(pa, L"ApplyOne<int>",							L"ApplyOne<int>",								L"{int $PR}"																	);
	AssertType(pa, L"ApplyOne<int, char>",						L"ApplyOne<int, char>"																											);
	AssertType(pa, L"ApplyOne<int, char, bool>",				L"ApplyOne<int, char, bool>"																									);
	AssertType(pa, L"ApplyTwo<>",								L"ApplyTwo<>",									L"{int * $PR, char * $PR}"														);
	AssertType(pa, L"ApplyTwo<int>",							L"ApplyTwo<int>",								L"{int $PR, char * $PR}"														);
	AssertType(pa, L"ApplyTwo<int, char>",						L"ApplyTwo<int, char>"							L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyTwo<int, char, bool>",				L"ApplyTwo<int, char, bool>"																									);
	AssertType(pa, L"ApplyVta<>",								L"ApplyVta<>",									L"{}"																			);
	AssertType(pa, L"ApplyVta<int>",							L"ApplyVta<int>",								L"{int $PR}"																	);
	AssertType(pa, L"ApplyVta<int, char>",						L"ApplyVta<int, char>",							L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyVta<int, char, bool>",				L"ApplyVta<int, char, bool>",					L"{int $PR, char $PR, bool $PR}"												);
	AssertType(pa, L"ApplyOneVta<>",							L"ApplyOneVta<>",								L"{}"																			);
	AssertType(pa, L"ApplyOneVta<int>",							L"ApplyOneVta<int>",							L"{int $PR}"																	);
	AssertType(pa, L"ApplyOneVta<int, char>",					L"ApplyOneVta<int, char>",						L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyOneVta<int, char, bool>",				L"ApplyOneVta<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);
	AssertType(pa, L"ApplyTwoVta<>",							L"ApplyTwoVta<>",								L"{}"																			);
	AssertType(pa, L"ApplyTwoVta<int>",							L"ApplyTwoVta<int>",							L"{int $PR}"																	);
	AssertType(pa, L"ApplyTwoVta<int, char>",					L"ApplyTwoVta<int, char>",						L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyTwoVta<int, char, bool>",				L"ApplyTwoVta<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);
	
	AssertType(pa, L"ApplyOne_1<>",								L"ApplyOne_1<>"																													);
	AssertType(pa, L"ApplyOne_1<int>",							L"ApplyOne_1<int>",								L"{int $PR}"																	);
	AssertType(pa, L"ApplyOne_1<int, char>",					L"ApplyOne_1<int, char>"																										);
	AssertType(pa, L"ApplyOne_1<int, char, bool>",				L"ApplyOne_1<int, char, bool>"																									);
	AssertType(pa, L"ApplyTwo_1<>",								L"ApplyTwo_1<>"																													);
	AssertType(pa, L"ApplyTwo_1<int>",							L"ApplyTwo_1<int>"								L"{int $PR, char * $PR}"														);
	AssertType(pa, L"ApplyTwo_1<int, char>",					L"ApplyTwo_1<int, char>"						L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyTwo_1<int, char, bool>",				L"ApplyTwo_1<int, char, bool>"																									);
	AssertType(pa, L"ApplyVta_1<>",								L"ApplyVta_1<>"																													);
	AssertType(pa, L"ApplyVta_1<int>",							L"ApplyVta_1<int>",								L"{int $PR}"																	);
	AssertType(pa, L"ApplyVta_1<int, char>",					L"ApplyVta_1<int, char>",						L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyVta_1<int, char, bool>",				L"ApplyVta_1<int, char, bool>",					L"{int $PR, char $PR, bool $PR}"												);
	AssertType(pa, L"ApplyOneVta_1<>",							L"ApplyOneVta_1<>"																												);
	AssertType(pa, L"ApplyOneVta_1<int>",						L"ApplyOneVta_1<int>",							L"{int $PR}"																	);
	AssertType(pa, L"ApplyOneVta_1<int, char>",					L"ApplyOneVta_1<int, char>",					L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyOneVta_1<int, char, bool>",			L"ApplyOneVta_1<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);
	AssertType(pa, L"ApplyTwoVta_1<>",							L"ApplyTwoVta_1<>"																												);
	AssertType(pa, L"ApplyTwoVta_1<int>",						L"ApplyTwoVta_1<int>",							L"{int $PR}"																	);
	AssertType(pa, L"ApplyTwoVta_1<int, char>",					L"ApplyTwoVta_1<int, char>",					L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyTwoVta_1<int, char, bool>",			L"ApplyTwoVta_1<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);
}