#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseValueAlias)
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

TEST_CASE(TestParseValueAlias_GenericExpr)
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

TEST_CASE(TestParserValueAlias_SimpleReplace)
{
	auto input = LR"(
template<typename T, T Value>
using Id = Value;

template<typename T, T Value>
using Parenthesis = (Value);

template<typename T, typename U, T Value>
using Cast = (U)Value;

template<typename T, T Value>
using Throw = throw Value;

template<typename T, T Value>
using New = new T(Value);

template<typename T, T Value>
using Delete = delete Value;

template<typename T>
using Child = T::Value;

template<typename T, T Value>
using Field = Value.field;

template<typename T, T Value>
using PtrField = Value->field;

template<typename T, T Value>
using Array = Value[0];

template<typename T, T Value>
using Func = Value(0);

template<typename T, T Value>
using Ctor = T(Value);

template<typename T, T Value>
using Init = {nullptr, Value, 0};

template<typename T, T Value>
using Postfix = {Value++, Value--};

template<typename T, T Value>
using Prefix = {++Value, --Value, &Value, *Value};

template<typename T, typename U, T ValueT, U ValueU>
using Binary1 = ValueT.*ValueU;

template<typename T, typename U, T ValueT, U ValueU>
using Binary2 = ValueT->*ValueU;

template<typename T, typename U, T ValueT, U ValueU>
using Binary3 = ValueT+ValueU;

template<typename T, typename U, T ValueT, U ValueU>
using Binary4 = ValueT, ValueU;

template<typename T, T Value>
using If = Value ? Value : Value;

struct S
{
	using Value = 0;
	double field;

	__int8 operator++();
	__int16 operator--();
	__int32 operator~();
	__int64 operator!();
	float operator+();
	double operator&();
	char operator*();
};
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"Id",				L"Id",							L"<::Id::[T], *> ::Id::[T] $PR",												pa);
	AssertExpr(L"Parenthesis",		L"Parenthesis",					L"<::Parenthesis::[T], *> ::Parenthesis::[T] $PR",								pa);
	AssertExpr(L"Cast",				L"Cast",						L"<::Cast::[T], ::Cast::[U], *> ::Cast::[U] $PR",								pa);
	AssertExpr(L"Throw",			L"Throw",						L"<::Throw::[T], *> void $PR",													pa);
	AssertExpr(L"New",				L"New",							L"<::New::[T], *> ::New::[T] * $PR",											pa);
	AssertExpr(L"Delete",			L"Delete",						L"<::Delete::[T], *> void $PR",													pa);
	AssertExpr(L"Child",			L"Child",						L"<::Child::[T]> any_t $PR",													pa);
	AssertExpr(L"Field",			L"Field",						L"<::Field::[T], *> any_t $PR",													pa);
	AssertExpr(L"PtrField",			L"PtrField",					L"<::PtrField::[T], *> any_t $PR",												pa);
	AssertExpr(L"Array",			L"Array",						L"<::Array::[T], *> any_t $PR",													pa);
	AssertExpr(L"Func",				L"Func",						L"<::Func::[T], *> any_t $PR",													pa);
	AssertExpr(L"Ctor",				L"Ctor",						L"<::Ctor::[T], *> ::Ctor::[T] $PR",											pa);
	AssertExpr(L"Init",				L"Init",						L"<::Init::[T], *> {nullptr_t $PR, ::Init::[T] $PR, 0 $PR} $PR",				pa);
	AssertExpr(L"Postfix",			L"Postfix",						L"<::Postfix::[T], *> {any_t $PR, any_t $PR} $PR",								pa);
	AssertExpr(L"Prefix",			L"Prefix",						L"<::Prefix::[T], *> {any_t $PR, any_t $PR, any_t $PR, any_t $PR} $PR",			pa);
	AssertExpr(L"Binary1",			L"Binary1",						L"<::Binary1::[T], ::Binary1::[U], *, *> any_t $PR",							pa);
	AssertExpr(L"Binary2",			L"Binary2",						L"<::Binary2::[T], ::Binary2::[U], *, *> any_t $PR",							pa);
	AssertExpr(L"Binary3",			L"Binary3",						L"<::Binary3::[T], ::Binary3::[U], *, *> any_t $PR",							pa);
	AssertExpr(L"Binary4",			L"Binary4",						L"<::Binary4::[T], ::Binary4::[U], *, *> any_t $PR",							pa);
	AssertExpr(L"If",				L"If",							L"<::If::[T], *> ::If::[T] $PR",												pa);

	AssertExpr(L"Id<int, 0>",										L"Id<int, 0>",												L"__int32 $PR",												pa);
	AssertExpr(L"Parenthesis<int, 0>",								L"Parenthesis<int, 0>",										L"__int32 $PR",												pa);
	AssertExpr(L"Cast<int, double, 0>",								L"Cast<int, double, 0>",									L"double $PR",												pa);
	AssertExpr(L"Throw<int, 0>",									L"Throw<int, 0>",											L"void $PR",												pa);
	AssertExpr(L"New<S, S()>",										L"New<S, S()>",												L"::S * $PR",												pa);
	AssertExpr(L"Delete<S*, nullptr>",								L"Delete<S *, nullptr>",									L"void $PR",												pa);
	AssertExpr(L"Child<S>",											L"Child<S>",												L"__int32 $PR",												pa);
	AssertExpr(L"Field<S, S()>",									L"Field<S, S()>",											L"double $PR",												pa);
	AssertExpr(L"PtrField<S*, nullptr>",							L"PtrField<S *, nullptr>",									L"double $PR",												pa);
	AssertExpr(L"Array<S*, nullptr>",								L"Array<S *, nullptr>",										L"::S & $PR",												pa);
	AssertExpr(L"Func<S(*)(), nullptr>",							L"Func<S () *, nullptr>",									L"::S $PR",													pa);
	AssertExpr(L"Ctor<S, S()>",										L"Ctor<S, S()>",											L"::S $PR",													pa);
	AssertExpr(L"Init<S, S()>",										L"Init<S, S()>",											L"{nullptr_t $PR, ::S $PR, 0 $PR} $PR",						pa);
	AssertExpr(L"Postfix<S*, nullptr>",								L"Postfix<S *, nullptr>",									L"{::S * $PR, ::S * $PR} $PR",								pa);
	AssertExpr(L"Prefix<S, S()>",									L"Prefix<S, S()>",											L"{__int8 $PR, __int16 $PR, double $PR, char $PR} $PR",		pa);
	AssertExpr(L"Binary1<S, double S::*, S(), &S::field>",			L"Binary1<S, double (S ::) *, S(), (& S :: field)>",		L"double & $PR",											pa);
	AssertExpr(L"Binary2<S*, double S::*, nullptr, &S::field>",		L"Binary2<S *, double (S ::) *, nullptr, (& S :: field)>",	L"double & $PR",											pa);
	AssertExpr(L"Binary3<int, double, 0, 0>",						L"Binary3<int, double, 0, 0>",								L"double $PR",												pa);
	AssertExpr(L"Binary4<int, double, 0, 0>",						L"Binary4<int, double, 0, 0>",								L"double $PR",												pa);
	AssertExpr(L"If<S, S()>",										L"If<S, S()>",												L"::S $PR",													pa);
}