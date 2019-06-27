#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseVariadicTemplateArgument_Apply)
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

TEST_CASE(TestParseVariadicTemplateArgument_Types)
{
	auto input = LR"(
struct A{ typedef int X; };
struct B{ typedef bool X; };
struct C{ typedef char X; };
struct D{ typedef double X; };
struct X{};
struct Y{};

template<typename R, typename... TArgs>
using Ref = R(*)(TArgs*&&...);

template<typename R, typename... TArgs>
using Array = R(*)(TArgs[10]...);

template<typename R, typename... TArgs>
using Child = R(*)(const volatile typename TArgs::X...);

template<typename R, typename... TArgs>
using Member1 = R(*)(X TArgs::*...);

template<typename R, typename... TArgs>
using Member2 = R(*)(TArgs Y::*...);

template<typename R, typename... TArgs>
using Member3 = R(*)(TArgs TArgs::*...);

template<typename R, typename... TArgs>
using Func1 = R(*)(TArgs(*)()...);

template<typename R, typename... TArgs>
using Func2 = R(*)(int(*)(TArgs)...);

template<typename R, typename... TArgs>
using Func3 = R(*)(TArgs(*)(TArgs)...);
)";
	// TODO: Test type passings by function types, unlike TestParseVariadicTemplateArgument_ApplyOn_VTA_Default by init expression
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"Ref",											L"Ref",											L"<::Ref::[R], ...::Ref::[TArgs]> any_t"																					);
	AssertType(pa, L"Array",										L"Array",										L"<::Array::[R], ...::Array::[TArgs]> any_t"																				);
	AssertType(pa, L"Child",										L"Child",										L"<::Child::[R], ...::Child::[TArgs]> any_t"																				);
	AssertType(pa, L"Member1",										L"Member1",										L"<::Member1::[R], ...::Member1::[TArgs]> any_t"																			);
	AssertType(pa, L"Member2",										L"Member2",										L"<::Member2::[R], ...::Member2::[TArgs]> any_t"																			);
	AssertType(pa, L"Member3",										L"Member3",										L"<::Member3::[R], ...::Member3::[TArgs]> any_t"																			);
	AssertType(pa, L"Func1",										L"Func1",										L"<::Func1::[R], ...::Func1::[TArgs]> any_t"																				);
	AssertType(pa, L"Func2",										L"Func2",										L"<::Func2::[R], ...::Func2::[TArgs]> any_t"																				);
	AssertType(pa, L"Func3",										L"Func3",										L"<::Func3::[R], ...::Func3::[TArgs]> any_t"																				);

	AssertType(pa, L"Ref<bool>",									L"Ref<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Ref<bool, int>",								L"Ref<bool, int>",								L"bool __cdecl(__int32 * &&) *"																								);
	AssertType(pa, L"Ref<bool, int, bool, char, double>",			L"Ref<bool, int, bool, char, double>",			L"bool __cdecl(__int32 * &&, bool * &&, char * &&, double * &&) *"															);

	AssertType(pa, L"Array<bool>",									L"Array<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Array<bool, int>",								L"Array<bool, int>",							L"bool __cdecl(__int32 []) *"																								);
	AssertType(pa, L"Array<bool, int, bool, char, double>",			L"Array<bool, int, bool, char, double>",		L"bool __cdecl(__int32 [], bool [], char [], double []) *"																	);

	AssertType(pa, L"Child<bool>",									L"Child<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Child<bool, A>",								L"Child<bool, A>",								L"bool __cdecl(__int32 const volatile) *"																					);
	AssertType(pa, L"Child<bool, A, B, C, D>",						L"Child<bool, A, B, C, D>",						L"bool __cdecl(__int32 const volatile, bool const volatile, char const volatile, double const volatile) *"					);

	AssertType(pa, L"Member1<bool>",								L"Member1<bool>",								L"bool __cdecl() *"																											);
	AssertType(pa, L"Member1<bool, A>",								L"Member1<bool, A>",							L"bool __cdecl(::X (::A ::) *) *"																							);
	AssertType(pa, L"Member1<bool, A, B, C, D>",					L"Member1<bool, A, B, C, D>",					L"bool __cdecl(::X (::A ::) *, ::X (::B ::) *, ::X (::C ::) *, ::X (::D ::) *) *"											);

	AssertType(pa, L"Member2<bool>",								L"Member2<bool>",								L"bool __cdecl() *"																											);
	AssertType(pa, L"Member2<bool, A>",								L"Member2<bool, A>",							L"bool __cdecl(::A (::Y ::) *) *"																							);
	AssertType(pa, L"Member2<bool, A, B, C, D>",					L"Member2<bool, A, B, C, D>",					L"bool __cdecl(::A (::Y ::) *, ::B (::Y ::) *, ::C (::Y ::) *, ::D (::Y ::) *) *"											);

	AssertType(pa, L"Member3<bool>",								L"Member3<bool>",								L"bool __cdecl() *"																											);
	AssertType(pa, L"Member3<bool, A>",								L"Member3<bool, A>",							L"bool __cdecl(::A (::A ::) *) *"																							);
	AssertType(pa, L"Member3<bool, A, B, C, D>",					L"Member3<bool, A, B, C, D>",					L"bool __cdecl(::A (::A ::) *, ::B (::B ::) *, ::C (::C ::) *, ::D (::D ::) *) *"											);

	AssertType(pa, L"Func1<bool>",									L"Func1<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Func1<bool, int>",								L"Func1<bool, int>",							L"bool __cdecl(__int32 __cdecl() *) *"																						);
	AssertType(pa, L"Func1<bool, int, bool, char, double>",			L"Func1<bool, int, bool, char, double>",		L"bool __cdecl(__int32 __cdecl() *, bool __cdecl() *, char __cdecl() *, double __cdecl() *) *"								);

	AssertType(pa, L"Func2<bool>",									L"Func2<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Func2<bool, int>",								L"Func2<bool, int>",							L"bool __cdecl(__int32 __cdecl(__int32) *) *"																				);
	AssertType(pa, L"Func2<bool, int, bool, char, double>",			L"Func2<bool, int, bool, char, double>",		L"bool __cdecl(__int32 __cdecl(__int32) *, __int32 __cdecl(bool) *, __int32 __cdecl(char) *, __int32 __cdecl(double) *) *"	);

	AssertType(pa, L"Func3<bool>",									L"Func3<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Func3<bool, int>",								L"Func3<bool, int>",							L"bool __cdecl(__int32 __cdecl(__int32) *) *"																				);
	AssertType(pa, L"Func3<bool, int, bool, char, double>",			L"Func3<bool, int, bool, char, double>",		L"bool __cdecl(__int32 __cdecl(__int32) *, bool __cdecl(bool) *, char __cdecl(char) *, double __cdecl(double) *) *"			);
}
/*
TEST_CASE(TestParseVariadicTemplateArgument_Exprs)
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

TEST_CASE(TestParseVariadicTemplateArgument_ApplyOn_VTA_Default)
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
*/