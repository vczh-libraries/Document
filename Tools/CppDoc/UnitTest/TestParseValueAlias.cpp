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
using Prefix = {++Value, --Value, ~Value, !Value, +Value, &Value, *Value};

template<typename T, typename U, T ValueT, U ValueU>
using Binary = {ValueT.*ValueU, ValueT->*ValueU, ValueT+ValueU, (ValueT, ValueU)};

template<typename T, T Value>
using If = Value ? Value : Value;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"Id",				L"Id",							L"<::Id::[T], *> ::Id::[T] $PR",													pa);
	AssertExpr(L"Parenthesis",		L"Parenthesis",					L"<::Parenthesis::[T], *> ::Parenthesis::[T] $PR",									pa);
	AssertExpr(L"Cast",				L"Cast",						L"<::Cast::[T], ::Cast::[U], *> ::Cast::[U] $PR",									pa);
	AssertExpr(L"Throw",			L"Throw",						L"<::Throw::[T], *> void $PR",														pa);
	AssertExpr(L"New",				L"New",							L"<::New::[T], *> ::New::[T] * $PR",												pa);
	AssertExpr(L"Delete",			L"Delete",						L"<::Delete::[T], *> void $PR",														pa);
	AssertExpr(L"Child",			L"Child",						L"<::Child::[T],> any_t $PR",														pa);
	AssertExpr(L"Field",			L"Field",						L"<::Field::[T], *> any_t $PR",														pa);
	AssertExpr(L"PtrField",			L"PtrField",					L"<::PtrField::[T], *> any_t $PR",													pa);
	AssertExpr(L"Array",			L"Array",						L"<::Array::[T], *> any_t $PR",														pa);
	AssertExpr(L"Func",				L"Func",						L"<::Func::[T], *> any_t $PR",														pa);
	AssertExpr(L"Ctor",				L"Ctor",						L"<::Ctor::[T], *> ::Ctor::[T] $PR",												pa);
	AssertExpr(L"Init",				L"Init",						L"<::Init::[T], *> {nullptr_t, ::Init::[T], __int32} $PR",							pa);
	AssertExpr(L"Postfix",			L"Postfix",						L"<::Postfix::[T], *> {any_t, any_t } $PR",											pa);
	AssertExpr(L"Prefix",			L"Prefix",						L"<::Prefix::[T], *> {any_t, any_t, any_t, any_t, any_t, any_t, any_t} $PR",		pa);
	AssertExpr(L"Binary",			L"Binary",						L"<::Binary::[T], ::Binary::[U], *, *> {any_t, any_t, any_t, any_t} $PR",			pa);
	AssertExpr(L"If",				L"If",							L"<::If::[T], *> ::Id::[T] $PR",													pa);
}