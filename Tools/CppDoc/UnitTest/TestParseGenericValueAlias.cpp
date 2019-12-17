#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Normal")
	{
		auto input = LR"(
template<typename T>
auto Size = sizeof(T);

template<typename T>
auto Ctor = T();

template<typename T, T Value>
auto Id = Value;

template<typename, bool>
auto True = true;
)";

		auto output = LR"(
template<typename T>
using_value Size: auto = sizeof(T);
template<typename T>
using_value Ctor: auto = T();
template<typename T, T Value>
using_value Id: auto = Value;
template<typename, bool>
using_value True: auto = true;
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		AssertExpr(pa, L"Size",				L"Size",				L"<::Size::[T]> unsigned __int32 $PR"	);
		AssertExpr(pa, L"Ctor",				L"Ctor",				L"<::Ctor::[T]> ::Ctor::[T] $PR"		);
		AssertExpr(pa, L"Id",				L"Id",					L"<::Id::[T], *> ::Id::[T] $PR"			);
		AssertExpr(pa, L"True",				L"True",				L"<::True::[], *> bool $PR"				);
	});

	TEST_CATEGORY(L"Incomplete types")
	{
		auto input = LR"(
template<typename T>
inline const __int8 A = sizeof(T);

template<typename T>
inline const __int16 B = sizeof(T);

template<typename T>
inline const __int32 C = sizeof(T);

template<typename T>
inline const __int64 D = sizeof(T);
)";

		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"A<int>",			L"A<int>",				L"__int8 const $PR"						);
		AssertExpr(pa, L"B<int>",			L"B<int>",				L"__int16 const $PR"					);
		AssertExpr(pa, L"C<int>",			L"C<int>",				L"__int32 const $PR"					);
		AssertExpr(pa, L"D<int>",			L"D<int>",				L"__int64 const $PR"					);
	});

	TEST_CATEGORY(L"Simple replacing")
	{
		auto input = LR"(
template<typename T, T Value>
auto Id = Value;

template<typename T, T Value>
auto Parenthesis = (Value);

template<typename T, typename U, T Value>
auto Cast = (U)Value;

template<typename T, T Value>
auto Throw = throw Value;

template<typename T, T Value>
auto New = new T(Value);

template<typename T, T Value>
auto Delete = delete Value;

template<typename T>
auto Child = T::Value;

template<typename T, T Value>
auto Field1 = Value.field;

template<typename T, T Value>
auto Field2 = Value.T::field;

template<typename T, T* Value>
auto PtrField1 = Value->field;

template<typename T, T* Value>
auto PtrField2 = Value->T::field;

template<typename T, T Value>
auto Array = Value[0];

template<typename T, T Value>
auto Func = Value(0);

template<typename T, T Value>
auto Ctor = T(Value);

template<typename T, T Value>
auto Init = {nullptr, Value, 0};

template<typename T, T Value>
auto Postfix = {Value++, Value--};

template<typename T, T Value>
auto Prefix = {++Value, --Value, &Value, *Value};

template<typename T, typename U, T ValueT, U ValueU>
auto Binary1 = ValueT.*ValueU;

template<typename T, typename U, T ValueT, U ValueU>
auto Binary2 = ValueT->*ValueU;

template<typename T, typename U, T ValueT, U ValueU>
auto Binary3 = ValueT+ValueU;

template<typename T, typename U, T ValueT, U ValueU>
auto Binary4 = ValueT, ValueU;

template<typename T, T Value>
auto If = Value ? Value : Value;

struct S
{
	static auto Value = 0;
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

		AssertExpr(pa, L"Id",												L"Id",														L"<::Id::[T], *> ::Id::[T] $PR"											);
		AssertExpr(pa, L"Parenthesis",										L"Parenthesis",												L"<::Parenthesis::[T], *> ::Parenthesis::[T] $PR"						);
		AssertExpr(pa, L"Cast",												L"Cast",													L"<::Cast::[T], ::Cast::[U], *> ::Cast::[U] $PR"						);
		AssertExpr(pa, L"Throw",											L"Throw",													L"<::Throw::[T], *> void $PR"											);
		AssertExpr(pa, L"New",												L"New",														L"<::New::[T], *> ::New::[T] * $PR"										);
		AssertExpr(pa, L"Delete",											L"Delete",													L"<::Delete::[T], *> void $PR"											);
		AssertExpr(pa, L"Child",											L"Child",													L"<::Child::[T]> any_t $PR"												);
		AssertExpr(pa, L"Field1",											L"Field1",													L"<::Field1::[T], *> any_t $PR"											);
		AssertExpr(pa, L"Field2",											L"Field2",													L"<::Field2::[T], *> any_t $PR"											);
		AssertExpr(pa, L"PtrField1",										L"PtrField1",												L"<::PtrField1::[T], *> any_t $PR"										);
		AssertExpr(pa, L"PtrField2",										L"PtrField2",												L"<::PtrField2::[T], *> any_t $PR"										);
		AssertExpr(pa, L"Array",											L"Array",													L"<::Array::[T], *> any_t $PR"											);
		AssertExpr(pa, L"Func",												L"Func",													L"<::Func::[T], *> any_t $PR"											);
		AssertExpr(pa, L"Ctor",												L"Ctor",													L"<::Ctor::[T], *> ::Ctor::[T] $PR"										);
		AssertExpr(pa, L"Init",												L"Init",													L"<::Init::[T], *> {nullptr_t $PR, ::Init::[T] $PR, 0 $PR} $PR"			);
		AssertExpr(pa, L"Postfix",											L"Postfix",													L"<::Postfix::[T], *> {any_t $PR, any_t $PR} $PR"						);
		AssertExpr(pa, L"Prefix",											L"Prefix",													L"<::Prefix::[T], *> {any_t $PR, any_t $PR, any_t $PR, any_t $PR} $PR"	);
		AssertExpr(pa, L"Binary1",											L"Binary1",													L"<::Binary1::[T], ::Binary1::[U], *, *> any_t $PR"						);
		AssertExpr(pa, L"Binary2",											L"Binary2",													L"<::Binary2::[T], ::Binary2::[U], *, *> any_t $PR"						);
		AssertExpr(pa, L"Binary3",											L"Binary3",													L"<::Binary3::[T], ::Binary3::[U], *, *> any_t $PR"						);
		AssertExpr(pa, L"Binary4",											L"Binary4",													L"<::Binary4::[T], ::Binary4::[U], *, *> any_t $PR"						);
		AssertExpr(pa, L"If",												L"If",														L"<::If::[T], *> ::If::[T] $PR"											);

		AssertExpr(pa, L"Id<int, 0>",										L"Id<int, 0>",												L"__int32 $PR"															);
		AssertExpr(pa, L"Parenthesis<int, 0>",								L"Parenthesis<int, 0>",										L"__int32 $PR"															);
		AssertExpr(pa, L"Cast<int, double, 0>",								L"Cast<int, double, 0>",									L"double $PR"															);
		AssertExpr(pa, L"Throw<int, 0>",									L"Throw<int, 0>",											L"void $PR"																);
		AssertExpr(pa, L"New<S, S()>",										L"New<S, S()>",												L"::S * $PR"															);
		AssertExpr(pa, L"Delete<S*, nullptr>",								L"Delete<S *, nullptr>",									L"void $PR"																);
		AssertExpr(pa, L"Child<S>",											L"Child<S>",												L"__int32 $PR"															);
		AssertExpr(pa, L"Field1<S, S()>",									L"Field1<S, S()>",											L"double $PR"															);
		AssertExpr(pa, L"Field2<S, S()>",									L"Field2<S, S()>",											L"double $PR"															);
		AssertExpr(pa, L"PtrField1<S, nullptr>",							L"PtrField1<S, nullptr>",									L"double $PR"															);
		AssertExpr(pa, L"PtrField2<S, nullptr>",							L"PtrField2<S, nullptr>",									L"double $PR"															);
		AssertExpr(pa, L"Array<S*, nullptr>",								L"Array<S *, nullptr>",										L"::S & $PR"															);
		AssertExpr(pa, L"Func<S(*)(int), nullptr>",							L"Func<S (int) *, nullptr>",								L"::S $PR"																);
		AssertExpr(pa, L"Ctor<S, S()>",										L"Ctor<S, S()>",											L"::S $PR"																);
		AssertExpr(pa, L"Init<S, S()>",										L"Init<S, S()>",											L"{nullptr_t $PR, ::S $PR, 0 $PR} $PR"									);
		AssertExpr(pa, L"Postfix<S*, nullptr>",								L"Postfix<S *, nullptr>",									L"{::S * $PR, ::S * $PR} $PR"											);
		AssertExpr(pa, L"Prefix<S, S()>",									L"Prefix<S, S()>",											L"{__int8 $PR, __int16 $PR, double $PR, char $PR} $PR"					);
		AssertExpr(pa, L"Binary1<S, double S::*, S(), &S::field>",			L"Binary1<S, double (S ::) *, S(), (& S :: field)>",		L"double & $PR"															);
		AssertExpr(pa, L"Binary2<S*, double S::*, nullptr, &S::field>",		L"Binary2<S *, double (S ::) *, nullptr, (& S :: field)>",	L"double & $PR"															);
		AssertExpr(pa, L"Binary3<int, double, 0, 0>",						L"Binary3<int, double, 0, 0>",								L"double $PR"															);
		AssertExpr(pa, L"Binary4<int, double, 0, 0>",						L"Binary4<int, double, 0, 0>",								L"double $PR"															);
		AssertExpr(pa, L"If<S, S()>",										L"If<S, S()>",												L"::S $PR"																);
	});

	TEST_CATEGORY(L"Overloading")
	{
		auto input = LR"(
int* F(int*);
double* F(double*);
void F(...);
char(&F(char(&)[10]))[10];

template<typename T, T* Value>
auto C = F(Value);
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"C",						L"C",							L"<::C::[T], *> __int32 * $PR",	L"<::C::[T], *> double * $PR",	L"<::C::[T], *> void $PR");

		AssertExpr(pa, L"C<int, nullptr>",			L"C<int, nullptr>",				L"__int32 * $PR"			);
		AssertExpr(pa, L"C<double, nullptr>",		L"C<double, nullptr>",			L"double * $PR"				);
		AssertExpr(pa, L"C<char, nullptr>",			L"C<char, nullptr>",			L"void $PR"					);
	});

	TEST_CATEGORY(L"Template")
	{
		auto input = LR"(
struct S
{
	static auto X = 0;

	template<typename T>
	static auto Y = static_cast<T*>(nullptr);
};

template<typename T>
auto X = T::X;

template<typename T>
auto Y = T::template Y<T>;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"X",						L"X",								L"<::X::[T]> any_t $PR"			);
		AssertExpr(pa, L"Y",						L"Y",								L"<::Y::[T]> any_t $PR"			);
		AssertExpr(pa, L"X<S>",						L"X<S>",							L"__int32 $PR"					);
		AssertExpr(pa, L"Y<S>",						L"Y<S>",							L"::S * $PR"					);
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
auto X = {T{}, TValue, U{}, V<T, U>{}};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"X<int>",							L"X<int>",								L"{__int32 $PR, __int32 $PR, char $PR, __int32 __cdecl(char) * $PR} $PR"		);
		AssertExpr(pa, L"X<bool>",							L"X<bool>",								L"{bool $PR, bool $PR, float $PR, bool __cdecl(float) * $PR} $PR"				);
		AssertExpr(pa, L"X<int, {}>",						L"X<int, {}>",							L"{__int32 $PR, __int32 $PR, char $PR, __int32 __cdecl(char) * $PR} $PR"		);
		AssertExpr(pa, L"X<bool, {}>",						L"X<bool, {}>",							L"{bool $PR, bool $PR, float $PR, bool __cdecl(float) * $PR} $PR"				);
		AssertExpr(pa, L"X<int, {}, void*>",				L"X<int, {}, void *>",					L"{__int32 $PR, __int32 $PR, void * $PR, __int32 __cdecl(void *) * $PR} $PR"	);
		AssertExpr(pa, L"X<bool, {}, void*>",				L"X<bool, {}, void *>",					L"{bool $PR, bool $PR, void * $PR, bool __cdecl(void *) * $PR} $PR"				);
		AssertExpr(pa, L"X<int, {}, void*, ReverseFunc>",	L"X<int, {}, void *, ReverseFunc>",		L"{__int32 $PR, __int32 $PR, void * $PR, void * __cdecl(__int32) * $PR} $PR"	);
		AssertExpr(pa, L"X<bool, {}, void*, ReverseFunc>",	L"X<bool, {}, void *, ReverseFunc>",	L"{bool $PR, bool $PR, void * $PR, void * __cdecl(bool) * $PR} $PR"				);
	});
}