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
	void*			operator++();
	A				operator++(int);
	float			operator+(int);
	double			operator,(int);
	A*				operator*(A);
};

struct B
{
	char*			operator++();
	B				operator++(int);
	int*			operator+(int);
	bool*			operator,(int);
	B*				operator*(B);
};

struct C1
{
	int*			operator[](int);
	bool*			operator[](bool);
	char*			operator[](char);
	double*			operator[](double);
	C1*				operator[](C1);

	int				x;
	static int		y;
};

struct C2
{
	C2*				operator[](C2);

	bool			x;
	static bool		y;
};

struct C3
{
	C3*				operator[](C3);

	char			x;
	static char		y;
};

struct C4
{
	C4*				operator[](C4);

	double			x;
	static double	y;
};

template<typename T>
T Value = T{};

template<typename ...TArgs>
using Cast = void(*)(decltype((TArgs)nullptr)...);

template<typename ...TArgs>
using Parenthesis = void(*)(decltype((Value<TArgs>))...);

template<typename ...TArgs>
using PrefixUnary = void(*)(decltype(++Value<TArgs>)...);

template<typename ...TArgs>
using PostfixUnary = void(*)(decltype(Value<TArgs>++)...);

template<typename ...TArgs>
using Binary1 = void(*)(decltype(Value<TArgs>+1)...);

template<typename ...TArgs>
using Binary2 = void(*)(decltype(Value<TArgs>,1)...);

template<typename ...TArgs>
using Binary3 = void(*)(decltype(Value<TArgs>*Value<TArgs>)...);

template<typename ...TArgs>
using Array1 = void(*)(decltype(Value<C1>[Value<TArgs>])...);

template<typename ...TArgs>
using Array2 = void(*)(decltype(Value<TArgs*>[0])...);

template<typename ...TArgs>
using Array3 = void(*)(decltype(Value<TArgs>[Value<TArgs>])...);

template<typename ...TArgs>
using Child = void(*)(decltype(&TArgs::x)..., decltype(TArgs::y)...);

template<typename ...TArgs>
using FieldId1 = void(*)(decltype(Value<TArgs>.x)...);

template<typename ...TArgs>
using FieldId2 = void(*)(decltype(Value<C1>.TArgs::x)...);

template<typename ...TArgs>
using FieldId3 = void(*)(decltype(Value<TArgs>.TArgs::x)...);

template<typename ...TArgs>
using FieldChild1 = void(*)(decltype(Value<TArgs*>->x)...);

template<typename ...TArgs>
using FieldChild2 = void(*)(decltype(Value<C1*>->TArgs::x)...);

template<typename ...TArgs>
using FieldChild3 = void(*)(decltype(Value<TArgs*>->TArgs::x)...);
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertType(pa, L"Cast",											L"Cast",										L"<...::Cast::[TArgs]> any_t"																								);
	AssertType(pa, L"Parenthesis",									L"Parenthesis",									L"<...::Parenthesis::[TArgs]> any_t"																						);
	AssertType(pa, L"PrefixUnary",									L"PrefixUnary",									L"<...::PrefixUnary::[TArgs]> any_t"																						);
	AssertType(pa, L"PostfixUnary",									L"PostfixUnary",								L"<...::PostfixUnary::[TArgs]> any_t"																						);
	AssertType(pa, L"Binary1",										L"Binary1",										L"<...::Binary1::[TArgs]> any_t"																							);
	AssertType(pa, L"Binary2",										L"Binary2",										L"<...::Binary2::[TArgs]> any_t"																							);
	AssertType(pa, L"Binary3",										L"Binary3",										L"<...::Binary3::[TArgs]> any_t"																							);
	AssertType(pa, L"Array1",										L"Array1",										L"<...::Array1::[TArgs]> any_t"																								);
	AssertType(pa, L"Array2",										L"Array2",										L"<...::Array2::[TArgs]> any_t"																								);
	AssertType(pa, L"Array3",										L"Array3",										L"<...::Array3::[TArgs]> any_t"																								);
	AssertType(pa, L"Child",										L"Child",										L"<...::Child::[TArgs]> any_t"																								);
	AssertType(pa, L"FieldId1",										L"FieldId1",									L"<...::FieldId1::[TArgs]> any_t"																							);
	AssertType(pa, L"FieldId2",										L"FieldId2",									L"<...::FieldId2::[TArgs]> any_t"																							);
	AssertType(pa, L"FieldId3",										L"FieldId3",									L"<...::FieldId3::[TArgs]> any_t"																							);
	AssertType(pa, L"FieldChild1",									L"FieldChild1",									L"<...::FieldChild1::[TArgs]> any_t"																						);
	AssertType(pa, L"FieldChild2",									L"FieldChild2",									L"<...::FieldChild2::[TArgs]> any_t"																						);
	AssertType(pa, L"FieldChild3",									L"FieldChild3",									L"<...::FieldChild3::[TArgs]> any_t"																						);
	
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
	
	AssertType(pa, L"Binary3<>",									L"Binary3<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Binary3<int>",									L"Binary3<int>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"Binary3<int, char, A, B>",						L"Binary3<int, char, A, B>",					L"void __cdecl(__int32, __int32, ::A *, ::B *) *"																			);
	
	AssertType(pa, L"Array1<>",										L"Array1<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Array1<int>",									L"Array1<int>",									L"void __cdecl(__int32 *) *"																								);
	AssertType(pa, L"Array1<int, bool, char, double>",				L"Array1<int, bool, char, double>",				L"void __cdecl(__int32 *, bool *, char *, double *) *"																		);
	
	AssertType(pa, L"Array2<>",										L"Array2<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Array2<int>",									L"Array2<int>",									L"void __cdecl(__int32 &) *"																								);
	AssertType(pa, L"Array2<int, bool, char, double>",				L"Array2<int, bool, char, double>",				L"void __cdecl(__int32 &, bool &, char &, double &) *"																		);
	
	AssertType(pa, L"Array3<>",										L"Array3<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"Array3<C1>",									L"Array3<C1>",									L"void __cdecl(::C1 *) *"																									);
	AssertType(pa, L"Array3<C1, C2, C3, C4>",						L"Array3<C1, C2, C3, C4>",						L"void __cdecl(::C1 *, ::C2 *, ::C3 *, ::C4 *) *"																			);
	
	AssertType(pa, L"Child<>",										L"Child<>",										L"void __cdecl() *"																											);
	AssertType(pa, L"Child<C1>",									L"Child<C1>",									L"void __cdecl(__int32 (::C1 ::) *, __int32) *"																				);
	AssertType(pa, L"Child<C1, C2, C3, C4>",						L"Child<C1, C2, C3, C4>",						L"void __cdecl(__int32 (::C1 ::) *, bool (::C2 ::) *, char (::C3 ::) *, double (::C4 ::) *, __int32, bool, char, double) *"	);
	
	AssertType(pa, L"FieldId1<>",									L"FieldId1<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"FieldId1<C1>",									L"FieldId1<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldId1<C1, C2, C3, C4>",						L"FieldId1<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"FieldId2<>",									L"FieldId2<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"FieldId2<C1>",									L"FieldId2<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldId2<C1, C2, C3, C4>",						L"FieldId2<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"FieldId3<>",									L"FieldId3<>",									L"void __cdecl() *"																											);
	AssertType(pa, L"FieldId3<C1>",									L"FieldId3<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldId3<C1, C2, C3, C4>",						L"FieldId3<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"FieldChild1<>",								L"FieldChild1<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"FieldChild1<C1>",								L"FieldChild1<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldChild1<C1, C2, C3, C4>",					L"FieldChild1<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"FieldChild2<>",								L"FieldChild2<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"FieldChild2<C1>",								L"FieldChild2<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldChild2<C1, C2, C3, C4>",					L"FieldChild2<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
	
	AssertType(pa, L"FieldChild3<>",								L"FieldChild3<>",								L"void __cdecl() *"																											);
	AssertType(pa, L"FieldChild3<C1>",								L"FieldChild3<C1>",								L"void __cdecl(__int32) *"																									);
	AssertType(pa, L"FieldChild3<C1, C2, C3, C4>",					L"FieldChild3<C1, C2, C3, C4>",					L"void __cdecl(__int32, bool, char, double) *"																				);
}

TEST_CASE(TestParseVariadicTemplateArgument_Exprs_Variant)
{
	auto input = LR"(
struct A{};
struct B{};
struct C{};
struct D{};

template<typename ...TArgs>
auto Init1 = {(TArgs)nullptr...};

template<typename ...TArgs>
auto Init2 = {{(TArgs)nullptr}...};

template<typename ...TArgs>
auto Ctor1 = {TArgs{1,2,3}...};

template<typename ...TArgs>
auto Ctor2 = {A(TArgs{}...)};

template<typename ...TArgs>
auto Ctor3 = {A(TArgs{})...};

template<typename ...TArgs>
auto Ctor4 = {TArgs(TArgs{})...};

template<typename ...TArgs>
auto New1 = {new TArgs{1,2,3}...};

template<typename ...TArgs>
auto New2 = {new A(TArgs{}...)};

template<typename ...TArgs>
auto New3 = {new A(TArgs{})...};

template<typename ...TArgs>
auto New4 = {new TArgs(TArgs{})...};
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"Init1",										L"Init1",										L"<...::Init1::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Init2",										L"Init2",										L"<...::Init2::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Ctor1",										L"Ctor1",										L"<...::Ctor1::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Ctor2",										L"Ctor2",										L"<...::Ctor2::[TArgs]> {::A $PR} $PR"																						);
	AssertExpr(pa, L"Ctor3",										L"Ctor3",										L"<...::Ctor3::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Ctor4",										L"Ctor4",										L"<...::Ctor4::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"New1",											L"New1",										L"<...::New1::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"New2",											L"New2",										L"<...::New2::[TArgs]> {::A * $PR} $PR"																						);
	AssertExpr(pa, L"New3",											L"New3",										L"<...::New3::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"New4",											L"New4",										L"<...::New4::[TArgs]> any_t $PR"																							);
	
	AssertExpr(pa, L"Init1<>",										L"Init1<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Init1<int>",									L"Init1<int>",									L"{__int32 $PR} $PR"																										);
	AssertExpr(pa, L"Init1<int, bool, char, double>",				L"Init1<int, bool, char, double>",				L"{__int32 $PR, bool $PR, char $PR, double $PR} $PR"																		);
	
	AssertExpr(pa, L"Init2<>",										L"Init2<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Init2<int>",									L"Init2<int>",									L"{{__int32 $PR} $PR} $PR"																									);
	AssertExpr(pa, L"Init2<int, bool, char, double>",				L"Init2<int, bool, char, double>",				L"{{__int32 $PR} $PR, {bool $PR} $PR, {char $PR} $PR, {double $PR} $PR} $PR"												);
	
	AssertExpr(pa, L"Ctor1<>",										L"Ctor1<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Ctor1<A>",										L"Ctor1<A>",									L"{::A $PR} $PR"																											);
	AssertExpr(pa, L"Ctor1<A, B, C, D>",							L"Ctor1<A, B, C, D>",							L"{::A $PR, ::B $PR, ::C $PR, ::D $PR} $PR"																					);
	
	AssertExpr(pa, L"Ctor2<>",										L"Ctor2<>",										L"{::A $PR} $PR"																											);
	AssertExpr(pa, L"Ctor2<A>",										L"Ctor2<A>",									L"{::A $PR} $PR"																											);
	AssertExpr(pa, L"Ctor2<A, B, C, D>",							L"Ctor2<A, B, C, D>",							L"{::A $PR} $PR"																											);
	
	AssertExpr(pa, L"Ctor3<>",										L"Ctor3<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Ctor3<A>",										L"Ctor3<A>",									L"{::A $PR} $PR"																											);
	AssertExpr(pa, L"Ctor3<A, B, C, D>",							L"Ctor3<A, B, C, D>",							L"{::A $PR, ::A $PR, ::A $PR, ::A $PR} $PR"																					);
	
	AssertExpr(pa, L"Ctor4<>",										L"Ctor4<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Ctor4<A>",										L"Ctor4<A>",									L"{::A $PR} $PR"																											);
	AssertExpr(pa, L"Ctor4<A, B, C, D>",							L"Ctor4<A, B, C, D>",							L"{::A $PR, ::B $PR, ::C $PR, ::D $PR} $PR"																					);
	
	AssertExpr(pa, L"Init2<>",										L"Init2<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Init2<int>",									L"Init2<int>",									L"{{__int32 $PR} $PR} $PR"																									);
	AssertExpr(pa, L"Init2<int, bool, char, double>",				L"Init2<int, bool, char, double>",				L"{{__int32 $PR} $PR, {bool $PR} $PR, {char $PR} $PR, {double $PR} $PR} $PR"												);
	
	AssertExpr(pa, L"New1<>",										L"New1<>",										L"{} $PR"																													);
	AssertExpr(pa, L"New1<A>",										L"New1<A>",										L"{::A * $PR} $PR"																											);
	AssertExpr(pa, L"New1<A, B, C, D>",								L"New1<A, B, C, D>",							L"{::A * $PR, ::B * $PR, ::C * $PR, ::D * $PR} $PR"																			);
	
	AssertExpr(pa, L"New2<>",										L"New2<>",										L"{::A * $PR} $PR"																											);
	AssertExpr(pa, L"New2<A>",										L"New2<A>",										L"{::A * $PR} $PR"																											);
	AssertExpr(pa, L"New2<A, B, C, D>",								L"New2<A, B, C, D>",							L"{::A * $PR} $PR"																											);
	
	AssertExpr(pa, L"New3<>",										L"New3<>",										L"{} $PR"																													);
	AssertExpr(pa, L"New3<A>",										L"New3<A>",										L"{::A * $PR} $PR"																											);
	AssertExpr(pa, L"New3<A, B, C, D>",								L"New3<A, B, C, D>",							L"{::A * $PR, ::A * $PR, ::A * $PR, ::A * $PR} $PR"																			);
	
	AssertExpr(pa, L"New4<>",										L"New4<>",										L"{} $PR"																													);
	AssertExpr(pa, L"New4<A>",										L"New4<A>",										L"{::A * $PR} $PR"																											);
	AssertExpr(pa, L"New4<A, B, C, D>",								L"New4<A, B, C, D>",							L"{::A * $PR, ::B * $PR, ::C * $PR, ::D * $PR} $PR"																			);
}

TEST_CASE(TestParseVariadicTemplateArgument_Exprs_Function)
{
	auto input = LR"(
namespace a { struct A{ static int*		G(void*); operator void*(); }; int		F(A); }
namespace b { struct B{ static bool*	G(void*); operator void*(); }; bool		F(B); }
namespace c { struct C{ static char*	G(void*); operator void*(); }; char		F(C); }
namespace d { struct D{ static double*	G(void*); operator void*(); }; double	F(D); }
void* F(...);

template<typename ...TArgs>
auto Func1 = {F(TArgs{})...};

template<typename ...TArgs>
auto Func2 = {TArgs::G(nullptr)...};

template<typename ...TArgs>
auto Func3 = {TArgs::G(TArgs{})...};

int H(void);
bool H(int);
char H(int, int);
double H(int, int, int);
void H(...);

template<int ...Args>
auto Func4 = H(-Args...);
)";
	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"Func1",										L"Func1",										L"<...::Func1::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Func2",										L"Func2",										L"<...::Func2::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Func3",										L"Func3",										L"<...::Func3::[TArgs]> any_t $PR"																							);
	AssertExpr(pa, L"Func4",										L"Func4",										L"<...*> __int32 $PR", L"<...*> bool $PR", L"<...*> char $PR", L"<...*> double $PR", L"<...*> void $PR"						);
	
	AssertExpr(pa, L"Func1<>",										L"Func1<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Func1<a::A>",									L"Func1<a :: A>",								L"{__int32 $PR} $PR"																										);
	AssertExpr(pa, L"Func1<a::A, b::B, c::C, d::D>",				L"Func1<a :: A, b :: B, c :: C, d :: D>",		L"{__int32 $PR, bool $PR, char $PR, double $PR} $PR"																		);
	
	AssertExpr(pa, L"Func2<>",										L"Func2<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Func2<a::A>",									L"Func2<a :: A>",								L"{__int32 * $PR} $PR"																										);
	AssertExpr(pa, L"Func2<a::A, b::B, c::C, d::D>",				L"Func2<a :: A, b :: B, c :: C, d :: D>",		L"{__int32 * $PR, bool * $PR, char * $PR, double * $PR} $PR"																);
	
	AssertExpr(pa, L"Func3<>",										L"Func3<>",										L"{} $PR"																													);
	AssertExpr(pa, L"Func3<a::A>",									L"Func3<a :: A>",								L"{__int32 * $PR} $PR"																										);
	AssertExpr(pa, L"Func3<a::A, b::B, c::C, d::D>",				L"Func3<a :: A, b :: B, c :: C, d :: D>",		L"{__int32 * $PR, bool * $PR, char * $PR, double * $PR} $PR"																);
	
	AssertExpr(pa, L"Func4<>",										L"Func4<>",										L"__int32 $PR"																												);
	AssertExpr(pa, L"Func4<1>",										L"Func4<1>",									L"bool $PR"																													);
	AssertExpr(pa, L"Func4<1,2>",									L"Func4<1, 2>",									L"char $PR"																													);
	AssertExpr(pa, L"Func4<1,2,3>",									L"Func4<1, 2, 3>",								L"double $PR"																												);
	AssertExpr(pa, L"Func4<1,2,3,4>",								L"Func4<1, 2, 3, 4>",							L"void $PR"																													);
}

TEST_CASE(TestParseVariadicTemplateArgument_ApplyOn_VTA_Default)
{
	return;
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
	
	AssertType(pa, L"One",										L"One",											L"<...::One::[Ts]> {any_t $PR}"													);
	AssertType(pa, L"Two",										L"Two",											L"<...::Two::[Ts]> {any_t $PR, any_t $PR}"										);
	AssertType(pa, L"Vta",										L"Vta",											L"<...::Vta::[Ts]> any_t"														);
	AssertType(pa, L"OneVta",									L"OneVta",										L"<...::OneVta::[Ts]> any_t"													);
	AssertType(pa, L"TwoVta",									L"TwoVta",										L"<...::TwoVta::[Ts]> any_t"													);
	
	AssertType(pa, L"ApplyOne",									L"ApplyOne",									L"<...::ApplyOne::[Ts]> {any_t $PR}"											);
	AssertType(pa, L"ApplyTwo",									L"ApplyTwo",									L"<...::ApplyTwo::[Ts]> {any_t $PR, any_t $PR}"									);
	AssertType(pa, L"ApplyVta",									L"ApplyVta",									L"<...::ApplyVta::[Ts]> any_t"													);
	AssertType(pa, L"ApplyOneVta",								L"ApplyOneVta",									L"<...::ApplyOneVta::[Ts]> any_t"												);
	AssertType(pa, L"ApplyTwoVta",								L"ApplyTwoVta",									L"<...::ApplyTwoVta::[Ts]> any_t"												);
	
	AssertType(pa, L"ApplyOne_1",								L"ApplyOne_1",									L"<::ApplyOne_1::[T], ::...ApplyOne_1::[Ts]> {any_t $PR}"						);
	AssertType(pa, L"ApplyTwo_1",								L"ApplyTwo_1",									L"<::ApplyTwo_1::[T], ::...ApplyTwo_1::[Ts]> {any_t $PR, any_t $PR}"			);
	AssertType(pa, L"ApplyVta_1",								L"ApplyVta_1",									L"<::ApplyVta_1::[T], ::...ApplyVta_1::[Ts]> any_t"								);
	AssertType(pa, L"ApplyOneVta_1",							L"ApplyOneVta_1",								L"<::ApplyOneVta_1::[T], ::...ApplyOneVta_1::[Ts]> any_t"						);
	AssertType(pa, L"ApplyTwoVta_1",							L"ApplyTwoVta_1",								L"<::ApplyTwoVta_1::[T], ::...ApplyTwoVta_1::[Ts]> any_t"						);
	
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

	AssertType(pa, L"ApplyOneVta<>",							L"ApplyOneVta<>",								L"{int * $PR}"																	);
	AssertType(pa, L"ApplyOneVta<int>",							L"ApplyOneVta<int>",							L"{int $PR}"																	);
	AssertType(pa, L"ApplyOneVta<int, char>",					L"ApplyOneVta<int, char>",						L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyOneVta<int, char, bool>",				L"ApplyOneVta<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);

	AssertType(pa, L"ApplyTwoVta<>",							L"ApplyTwoVta<>",								L"{int * $PR, char * $PR}"														);
	AssertType(pa, L"ApplyTwoVta<int>",							L"ApplyTwoVta<int>",							L"{int $PR, char * $PR}"														);
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
	AssertType(pa, L"ApplyTwoVta_1<int>",						L"ApplyTwoVta_1<int>",							L"{int $PR, char* $PR}"															);
	AssertType(pa, L"ApplyTwoVta_1<int, char>",					L"ApplyTwoVta_1<int, char>",					L"{int $PR, char $PR}"															);
	AssertType(pa, L"ApplyTwoVta_1<int, char, bool>",			L"ApplyTwoVta_1<int, char, bool>",				L"{int $PR, char $PR, bool $PR}"												);
}
