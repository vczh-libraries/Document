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

template<typename T>
using PtrLRef = T*&&;

template<typename R, typename... TArgs>
using Ref = R(*)(TArgs*&&...);

template<typename R, typename... TArgs>
using Ref2 = R(*)(PtrLRef<TArgs>...);

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
	AssertType(pa, L"Ref2",											L"Ref2",										L"<::Ref2::[R], ...::Ref2::[TArgs]> any_t"																					);
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

	AssertType(pa, L"Ref2<bool>",									L"Ref2<bool>",									L"bool __cdecl() *"																											);
	AssertType(pa, L"Ref2<bool, int>",								L"Ref2<bool, int>",								L"bool __cdecl(__int32 * &&) *"																								);
	AssertType(pa, L"Ref2<bool, int, bool, char, double>",			L"Ref2<bool, int, bool, char, double>",			L"bool __cdecl(__int32 * &&, bool * &&, char * &&, double * &&) *"															);

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

TEST_CASE(TestParseVariadicTemplateArgument_Exprs_Unbounded)
{
	auto input = LR"(

struct A
{
	void*	operator++();
	A		operator++(int);
	float	operator+(int);
	double	operator,(int);
};

struct B
{
	char*	operator++();
	B		operator++(int);
	int*	operator+(int);
	bool*	operator,(int);
};

template<typename ...TArgs>
using Cast = void(*)(decltype((TArgs)nullptr)...);

template<typename ...TArgs>
using Parenthesis = void(*)(decltype(((TArgs)nullptr))...);

template<typename ...TArgs>
using PrefixUnary = void(*)(decltype(++(TArgs)nullptr)...);

template<typename ...TArgs>
using PostfixUnary = void(*)(decltype(((TArgs)nullptr)++)...);

template<typename ...TArgs>
using Binary1 = void(*)(decltype(((TArgs)nullptr)+1)...);

template<typename ...TArgs>
using Binary2 = void(*)(decltype(((TArgs)nullptr),1)...);
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"Cast",											L"Cast",										L"<...::Cast::[TArgs]> any_t"																								);
	AssertType(pa, L"Parenthesis",									L"Parenthesis",									L"<...::Parenthesis::[TArgs]> any_t"																						);
	AssertType(pa, L"PrefixUnary",									L"PrefixUnary",									L"<...::PrefixUnary::[TArgs]> any_t"																						);
	AssertType(pa, L"PostfixUnary",									L"PostfixUnary",								L"<...::PostfixUnary::[TArgs]> any_t"																						);
	AssertType(pa, L"Binary1",										L"Binary1",										L"<...::Binary1::[TArgs]> any_t"																							);
	AssertType(pa, L"Binary2",										L"Binary2",										L"<...::Binary2::[TArgs]> any_t"																							);
	
	AssertType(pa, L"Cast<>",										L"Cast<>",										L"void __cdecl() *"																											);
	AssertType(pa, L"Cast<int>",									L"Cast<int>",									L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"Cast<int, bool, char, double>",				L"Cast<int, bool, char, double>",				L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"Parenthesis<>",								L"Parenthesis<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"Parenthesis<int>",								L"Parenthesis<int>",							L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"Parenthesis<int, bool, char, double>",			L"Parenthesis<int, bool, char, double>",		L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"PrefixUnary<>",								L"PrefixUnary<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"PrefixUnary<int>",								L"PrefixUnary<int>",							L"void __cdecl(__int32 &) *"																								);
	AssertType(pa, L"PrefixUnary<int, char, A, B>",					L"PrefixUnary<int, char, A, B>",				L"void __cdecl(__int32 &, char &, void *, char *) *"																		);
	
	AssertType(pa, L"PostfixUnary<>",								L"PostfixUnary<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"PostfixUnary<int>",							L"PostfixUnary<int>",							L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"PostfixUnary<int, char, A, B>",				L"PostfixUnary<int, char, A, B>",				L"void __cdecl(__int32, char, ::A, ::B) *"																					);
	
	AssertType(pa, L"Binary1<>",									L"Binary1<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Binary1<int>",									L"Binary1<int>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"Binary1<int, char, A, B>",						L"Binary1<int, char, A, B>",					L"void __cdecl(__int32, __int32, float, __int32 *) *"																		);
	
	AssertType(pa, L"Binary2<>",									L"Binary2<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Binary2<int>",									L"Binary2<int>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"Binary2<int, char, A, B>",						L"Binary2<int, char, A, B>",					L"void __cdecl(__int32, __int32, double, bool *) *"																			);
}

/*
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