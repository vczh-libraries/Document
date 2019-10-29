#include "Util.h"

TEST_CASE(TestParseGenericMember_FFA)
{
	auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct Y
{
	Tx x;
	Ty y;

	X<Tx*, Ty&>* operator->();
};

template<typename Tx, typename Ty>
struct Z
{
	Tx x;
	Ty y;

	Y<const Tx, volatile Ty> operator->();
	X<const Tx, volatile Ty> operator()(int);
	Y<const Tx, volatile Ty> operator()(void*);
	X<const Tx, volatile Ty> operator[](const char*);
	Y<const Tx, volatile Ty> operator[](Z<Tx, Ty>&&);

	static Tx F(double);
	Ty G(void*);
};

X<char, wchar_t> (*x)(int) = nullptr;
Y<float, double> (*const y)(int) = nullptr;

X<char, wchar_t> (*(*x2)(void*))(int) = nullptr;
Y<float, double> (*const (*const y2)(void*))(int) = nullptr;

Z<void*, char*> z;
Z<void*, char*>* pz = nullptr;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"pz->x",									L"pz->x",										L"void * $L"													);
	AssertExpr(pa, L"pz->y",									L"pz->y",										L"char * $L"													);
	AssertExpr(pa, L"pz->F",									L"pz->F",										L"void * __cdecl(double) * $PR"									);
	AssertExpr(pa, L"pz->G",									L"pz->G",										L"char * __thiscall(void *) * $PR"								);
	AssertExpr(pa, L"pz->operator->",							L"pz->operator ->",								L"::Y<void * const, char * volatile> __thiscall() * $PR"		);

	AssertExpr(pa, L"z.x",										L"z.x",											L"void * $L"													);
	AssertExpr(pa, L"z.y",										L"z.y",											L"char * $L"													);
	AssertExpr(pa, L"z.F",										L"z.F",											L"void * __cdecl(double) * $PR"									);
	AssertExpr(pa, L"z.G",										L"z.G",											L"char * __thiscall(void *) * $PR"								);
	AssertExpr(pa, L"z.operator->",								L"z.operator ->",								L"::Y<void * const, char * volatile> __thiscall() * $PR"		);

	AssertExpr(pa, L"pz->operator->()",							L"pz->operator ->()",							L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"pz->operator()(0)",						L"pz->operator ()(0)",							L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"pz->operator()(nullptr)",					L"pz->operator ()(nullptr)",					L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"pz->operator[](\"a\")",					L"pz->operator [](\"a\")",						L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"pz->operator[](Z<void*, char*>())",		L"pz->operator [](Z<void *, char *>())",		L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"pz->F(0)",									L"pz->F(0)",									L"void * $PR"													);
	AssertExpr(pa, L"pz->G(0)",									L"pz->G(0)",									L"char * $PR"													);

	AssertExpr(pa, L"z.operator->()",							L"z.operator ->()",								L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z.operator()(0)",							L"z.operator ()(0)",							L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z.operator()(nullptr)",					L"z.operator ()(nullptr)",						L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z.operator[](\"a\")",						L"z.operator [](\"a\")",						L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z.operator[](Z<void*, char*>())",			L"z.operator [](Z<void *, char *>())",			L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z.F(0)",									L"z.F(0)",										L"void * $PR"													);
	AssertExpr(pa, L"z.G(0)",									L"z.G(0)",										L"char * $PR"													);

	AssertExpr(pa, L"z->x",										L"z->x",										L"void * const * $L"											);
	AssertExpr(pa, L"z->y",										L"z->y",										L"char * volatile & $L"											);
	AssertExpr(pa, L"z(0)",										L"z(0)",										L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z(nullptr)",								L"z(nullptr)",									L"::Y<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z[\"a\"]",									L"z[\"a\"]",									L"::X<void * const, char * volatile> $PR"						);
	AssertExpr(pa, L"z[Z<void*, char*>()]",						L"z[Z<void *, char *>()]",						L"::Y<void * const, char * volatile> $PR"						);

	AssertExpr(pa, L"x(0)",										L"x(0)",										L"::X<char, wchar_t> $PR"										);
	AssertExpr(pa, L"y(0)",										L"y(0)",										L"::Y<float, double> $PR"										);
	AssertExpr(pa, L"x2(nullptr)(0)",							L"x2(nullptr)(0)",								L"::X<char, wchar_t> $PR"										);
	AssertExpr(pa, L"y2(nullptr)(0)",							L"y2(nullptr)(0)",								L"::Y<float, double> $PR"										);
}

TEST_CASE(TestParseGenericMember_Field_Qualifier)
{
	auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

X<int, double> x;
X<int, double>& lx;
X<int, double>&& rx;

const X<int, double> cx;
const X<int, double>& clx;
const X<int, double>&& crx;

X<int, double> F();
X<int, double>& lF();
X<int, double>&& rF();

const X<int, double> cF();
const X<int, double>& clF();
const X<int, double>&& crF();

X<int, double>* px;
const X<int, double>* cpx;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"x",						L"x",							L"::X<__int32, double> $L"					);
	AssertExpr(pa, L"lx",						L"lx",							L"::X<__int32, double> & $L"				);
	AssertExpr(pa, L"rx",						L"rx",							L"::X<__int32, double> && $L"				);
	AssertExpr(pa, L"cx",						L"cx",							L"::X<__int32, double> const $L"			);
	AssertExpr(pa, L"clx",						L"clx",							L"::X<__int32, double> const & $L"			);
	AssertExpr(pa, L"crx",						L"crx",							L"::X<__int32, double> const && $L"			);
	AssertExpr(pa, L"px",						L"px",							L"::X<__int32, double> * $L"				);
	AssertExpr(pa, L"cpx",						L"cpx",							L"::X<__int32, double> const * $L"			);

	AssertExpr(pa, L"x.x",						L"x.x",							L"__int32 $L"								);
	AssertExpr(pa, L"lx.x",						L"lx.x",						L"__int32 $L"								);
	AssertExpr(pa, L"rx.x",						L"rx.x",						L"__int32 $L"								);
	AssertExpr(pa, L"cx.x",						L"cx.x",						L"__int32 const $L"							);
	AssertExpr(pa, L"clx.x",					L"clx.x",						L"__int32 const $L"							);
	AssertExpr(pa, L"crx.x",					L"crx.x",						L"__int32 const $L"							);

	AssertExpr(pa, L"x.y",						L"x.y",							L"double $L"								);
	AssertExpr(pa, L"lx.y",						L"lx.y",						L"double $L"								);
	AssertExpr(pa, L"rx.y",						L"rx.y",						L"double $L"								);
	AssertExpr(pa, L"cx.y",						L"cx.y",						L"double const $L"							);
	AssertExpr(pa, L"clx.y",					L"clx.y",						L"double const $L"							);
	AssertExpr(pa, L"crx.y",					L"crx.y",						L"double const $L"							);

	AssertExpr(pa, L"F().x",					L"F().x",						L"__int32 $PR"								);
	AssertExpr(pa, L"lF().x",					L"lF().x",						L"__int32 $L"								);
	AssertExpr(pa, L"rF().x",					L"rF().x",						L"__int32 && $X"							);
	AssertExpr(pa, L"cF().x",					L"cF().x",						L"__int32 const $PR"						);
	AssertExpr(pa, L"clF().x",					L"clF().x",						L"__int32 const $L"							);
	AssertExpr(pa, L"crF().x",					L"crF().x",						L"__int32 const && $X"						);
	AssertExpr(pa, L"px->x",					L"px->x",						L"__int32 $L"								);
	AssertExpr(pa, L"cpx->x",					L"cpx->x",						L"__int32 const $L"							);

	AssertExpr(pa, L"F().y",					L"F().y",						L"double $PR"								);
	AssertExpr(pa, L"lF().y",					L"lF().y",						L"double $L"								);
	AssertExpr(pa, L"rF().y",					L"rF().y",						L"double && $X"								);
	AssertExpr(pa, L"cF().y",					L"cF().y",						L"double const $PR"							);
	AssertExpr(pa, L"clF().y",					L"clF().y",						L"double const $L"							);
	AssertExpr(pa, L"crF().y",					L"crF().y",						L"double const && $X"						);
	AssertExpr(pa, L"px->y",					L"px->y",						L"double $L"								);
	AssertExpr(pa, L"cpx->y",					L"cpx->y",						L"double const $L"							);
}

TEST_CASE(TestParseGenericMember_FFA_Qualifier)
{
	auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct Z
{
	X<volatile Tx, const Ty>* operator->()const;
	volatile X<const Tx, volatile Ty>* operator->();
	X<volatile Tx, const Ty> operator()(int)const;
	X<const Tx, volatile Ty> operator()(int);
	X<volatile Tx, const Ty> operator[](int)const;
	X<const Tx, volatile Ty> operator[](int);
};

Z<int, double> z;
const Z<int, double> cz;

Z<int, double>& lz;
const Z<int, double>& lcz;

Z<int, double>&& rz;
const Z<int, double>&& rcz;

Z<int, double>* const pz;
const Z<int, double>* const pcz;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"z.operator->()",			L"z.operator ->()",				L"::X<__int32 const, double volatile> volatile * $PR"		);
	AssertExpr(pa, L"z.operator()(0)",			L"z.operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"z.operator[](0)",			L"z.operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"z->x",						L"z->x",						L"__int32 const volatile $L"								);
	AssertExpr(pa, L"z(0)",						L"z(0)",						L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"z[0]",						L"z[0]",						L"::X<__int32 const, double volatile> $PR"					);

	AssertExpr(pa, L"cz.operator->()",			L"cz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
	AssertExpr(pa, L"cz.operator()(0)",			L"cz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"cz.operator[](0)",			L"cz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"cz->x",					L"cz->x",						L"__int32 volatile $L"										);
	AssertExpr(pa, L"cz(0)",					L"cz(0)",						L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"cz[0]",					L"cz[0]",						L"::X<__int32 volatile, double const> $PR"					);

	AssertExpr(pa, L"lz.operator->()",			L"lz.operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
	AssertExpr(pa, L"lz.operator()(0)",			L"lz.operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"lz.operator[](0)",			L"lz.operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"lz->x",					L"lz->x",						L"__int32 const volatile $L"								);
	AssertExpr(pa, L"lz(0)",					L"lz(0)",						L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"lz[0]",					L"lz[0]",						L"::X<__int32 const, double volatile> $PR"					);

	AssertExpr(pa, L"lcz.operator->()",			L"lcz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
	AssertExpr(pa, L"lcz.operator()(0)",		L"lcz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"lcz.operator[](0)",		L"lcz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"lcz->x",					L"lcz->x",						L"__int32 volatile $L"										);
	AssertExpr(pa, L"lcz(0)",					L"lcz(0)",						L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"lcz[0]",					L"lcz[0]",						L"::X<__int32 volatile, double const> $PR"					);

	AssertExpr(pa, L"rz.operator->()",			L"rz.operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
	AssertExpr(pa, L"rz.operator()(0)",			L"rz.operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"rz.operator[](0)",			L"rz.operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"rz->x",					L"rz->x",						L"__int32 const volatile $L"								);
	AssertExpr(pa, L"rz(0)",					L"rz(0)",						L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"rz[0]",					L"rz[0]",						L"::X<__int32 const, double volatile> $PR"					);

	AssertExpr(pa, L"rcz.operator->()",			L"rcz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
	AssertExpr(pa, L"rcz.operator()(0)",		L"rcz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"rcz.operator[](0)",		L"rcz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"rcz->x",					L"rcz->x",						L"__int32 volatile $L"										);
	AssertExpr(pa, L"rcz(0)",					L"rcz(0)",						L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"rcz[0]",					L"rcz[0]",						L"::X<__int32 volatile, double const> $PR"					);

	AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
	AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
	AssertExpr(pa, L"pz->operator[](0)",		L"pz->operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);

	AssertExpr(pa, L"pcz->operator->()",		L"pcz->operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
	AssertExpr(pa, L"pcz->operator()(0)",		L"pcz->operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
	AssertExpr(pa, L"pcz->operator[](0)",		L"pcz->operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
}

Symbol* TestParseGenericMember_InsideFunction(const ParsingArguments& pa, const WString& className, const WString& methodName)
{
	auto classSymbol = pa.scopeSymbol->TryGetChildren_NFb(className)->Get(0);
	auto functionSymbol = classSymbol->TryGetChildren_NFb(methodName)->Get(0);
	auto functionBodySymbol = functionSymbol->GetImplSymbols_F()[0];
	return functionBodySymbol->TryGetChildren_NFb(L"$")->Get(0).Obj();
}

TEST_CASE(TestParseGenericMember_FFA_Qualifier_OfExplicitOrImplicitThisExpr)
{
	auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct Z
{
	X<volatile Tx, const Ty>* operator->()const;
	volatile X<const Tx, volatile Ty>* operator->();
	X<volatile Tx, const Ty> operator()(int)const;
	X<const Tx, volatile Ty> operator()(int);
	X<volatile Tx, const Ty> operator[](int)const;
	X<const Tx, volatile Ty> operator[](int);

	void M(){}
	void C()const{}
};
)";
	COMPILE_PROGRAM(program, pa, input);

	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"Z", L"M"));

		AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::X<::Z::[Tx] const, ::Z::[Ty] volatile> volatile * $PR"		);
		AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::X<::Z::[Tx] const, ::Z::[Ty] volatile> $PR"					);
		AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::X<::Z::[Tx] const, ::Z::[Ty] volatile> $PR"					);
		AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"::Z::[Tx] const volatile $L"									);
		AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::X<::Z::[Tx] const, ::Z::[Ty] volatile> $PR"					);
		AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::X<::Z::[Tx] const, ::Z::[Ty] volatile> $PR"					);
	}
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"Z", L"C"));

		AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::X<::Z::[Tx] volatile, ::Z::[Ty] const> * $PR"				);
		AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::X<::Z::[Tx] volatile, ::Z::[Ty] const> $PR"					);
		AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::X<::Z::[Tx] volatile, ::Z::[Ty] const> $PR"					);
		AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"::Z::[Tx] volatile $L"										);
		AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::X<::Z::[Tx] volatile, ::Z::[Ty] const> $PR"					);
		AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::X<::Z::[Tx] volatile, ::Z::[Ty] const> $PR"					);
	}
}

TEST_CASE(TestParseGenericMember_AddressOfArrayFunctionMemberPointer)
{
	auto input = LR"(
template<typename TA, typename TB, typename TC, typename TD>
struct X
{
	static TA A;
	TB B;
	static TC C();
	TD D();
};

struct Y : X<char, bool, float, double>
{
};

template<typename T>
T* E();

auto				_A1 = &Y::A;
auto				_B1 = &Y::B;
auto				_C1 = &Y::C;
auto				_D1 = &Y::D;
auto				_E1 = &E<void>;

decltype(auto)		_A2 = &Y::A;
decltype(auto)		_B2 = &Y::B;
decltype(auto)		_C2 = &Y::C;
decltype(auto)		_D2 = &Y::D;
decltype(auto)		_E2 = &E<void>;

decltype(&Y::A)		_A3[1] = {&Y::A};
decltype(&Y::B)		_B3[1] = {&Y::B};
decltype(&Y::C)		_C3[1] = {&Y::C};
decltype(&Y::D)		_D3[1] = {&Y::D};
decltype(&E<void>)	_E3[1] = {&E<void>};

auto&&				_A4 = &Y::A;
auto&&				_B4 = &Y::B;
auto&&				_C4 = &Y::C;
auto&&				_D4 = &Y::D;
auto&&				_E4 = &E<void>;

const auto&			_A5 = &Y::A;
const auto&			_B5 = &Y::B;
const auto&			_C5 = &Y::C;
const auto&			_D5 = &Y::D;
const auto&			_E5 = &E<void>;

auto&				_A6 = _A5;
auto&				_B6 = _B5;
auto&				_C6 = _C5;
auto&				_D6 = _D5;
auto&				_E6 = _E5;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"Y::A",			L"Y :: A",				L"char $L"																	);
	AssertExpr(pa, L"Y::B",			L"Y :: B",				L"bool $L"																	);
	AssertExpr(pa, L"Y::C",			L"Y :: C",				L"float __cdecl() * $PR"													);
	AssertExpr(pa, L"Y::D",			L"Y :: D",				L"double __thiscall() * $PR"												);
	AssertExpr(pa, L"E",			L"E",					L"<::E::[T]> ::E::[T] * __cdecl() * $PR"									);

	AssertExpr(pa, L"&Y::A",		L"(& Y :: A)",			L"char * $PR"																);
	AssertExpr(pa, L"&Y::B",		L"(& Y :: B)",			L"bool (::X<char, bool, float, double> ::) * $PR"							);
	AssertExpr(pa, L"&Y::C",		L"(& Y :: C)",			L"float __cdecl() * $PR"													);
	AssertExpr(pa, L"&Y::D",		L"(& Y :: D)",			L"double __thiscall() (::X<char, bool, float, double> ::) * $PR"			);
	AssertExpr(pa, L"&E<void>",		L"(& E<void>)",			L"void * __cdecl() * $PR"													);

	AssertExpr(pa, L"_A1",			L"_A1",					L"char * $L"																);
	AssertExpr(pa, L"_B1",			L"_B1",					L"bool (::X<char, bool, float, double> ::) * $L"							);
	AssertExpr(pa, L"_C1",			L"_C1",					L"float __cdecl() * $L"														);
	AssertExpr(pa, L"_D1",			L"_D1",					L"double __thiscall() (::X<char, bool, float, double> ::) * $L"				);
	AssertExpr(pa, L"_E1",			L"_E1",					L"void * __cdecl() * $L"													);

	AssertExpr(pa, L"&_A1",			L"(& _A1)",				L"char * * $PR"																);
	AssertExpr(pa, L"&_B1",			L"(& _B1)",				L"bool (::X<char, bool, float, double> ::) * * $PR"							);
	AssertExpr(pa, L"&_C1",			L"(& _C1)",				L"float __cdecl() * * $PR"													);
	AssertExpr(pa, L"&_D1",			L"(& _D1)",				L"double __thiscall() (::X<char, bool, float, double> ::) * * $PR"			);
	AssertExpr(pa, L"&_E1",			L"(& _E1)",				L"void * __cdecl() * * $PR"													);

	AssertExpr(pa, L"_A2",			L"_A2",					L"char * $L"																);
	AssertExpr(pa, L"_B2",			L"_B2",					L"bool (::X<char, bool, float, double> ::) * $L"							);
	AssertExpr(pa, L"_C2",			L"_C2",					L"float __cdecl() * $L"														);
	AssertExpr(pa, L"_D2",			L"_D2",					L"double __thiscall() (::X<char, bool, float, double> ::) * $L"				);
	AssertExpr(pa, L"_E2",			L"_E2",					L"void * __cdecl() * $L"													);

	AssertExpr(pa, L"&_A2",			L"(& _A2)",				L"char * * $PR"																);
	AssertExpr(pa, L"&_B2",			L"(& _B2)",				L"bool (::X<char, bool, float, double> ::) * * $PR"							);
	AssertExpr(pa, L"&_C2",			L"(& _C2)",				L"float __cdecl() * * $PR"													);
	AssertExpr(pa, L"&_D2",			L"(& _D2)",				L"double __thiscall() (::X<char, bool, float, double> ::) * * $PR"			);
	AssertExpr(pa, L"&_E2",			L"(& _E2)",				L"void * __cdecl() * * $PR"													);

	AssertExpr(pa, L"_A3",			L"_A3",					L"char * [] $L"																);
	AssertExpr(pa, L"_B3",			L"_B3",					L"bool (::X<char, bool, float, double> ::) * [] $L"							);
	AssertExpr(pa, L"_C3",			L"_C3",					L"float __cdecl() * [] $L"													);
	AssertExpr(pa, L"_D3",			L"_D3",					L"double __thiscall() (::X<char, bool, float, double> ::) * [] $L"			);
	AssertExpr(pa, L"_E3",			L"_E3",					L"void * __cdecl() * [] $L"													);

	AssertExpr(pa, L"&_A3",			L"(& _A3)",				L"char * [] * $PR"															);
	AssertExpr(pa, L"&_B3",			L"(& _B3)",				L"bool (::X<char, bool, float, double> ::) * [] * $PR"						);
	AssertExpr(pa, L"&_C3",			L"(& _C3)",				L"float __cdecl() * [] * $PR"												);
	AssertExpr(pa, L"&_D3",			L"(& _D3)",				L"double __thiscall() (::X<char, bool, float, double> ::) * [] * $PR"		);
	AssertExpr(pa, L"&_E3",			L"(& _E3)",				L"void * __cdecl() * [] * $PR"												);

	AssertExpr(pa, L"_A4",			L"_A4",					L"char * && $L"																);
	AssertExpr(pa, L"_B4",			L"_B4",					L"bool (::X<char, bool, float, double> ::) * && $L"							);
	AssertExpr(pa, L"_C4",			L"_C4",					L"float __cdecl() * && $L"													);
	AssertExpr(pa, L"_D4",			L"_D4",					L"double __thiscall() (::X<char, bool, float, double> ::) * && $L"			);
	AssertExpr(pa, L"_E4",			L"_E4",					L"void * __cdecl() * && $L"													);

	AssertExpr(pa, L"&_A4",			L"(& _A4)",				L"char * * $PR"																);
	AssertExpr(pa, L"&_B4",			L"(& _B4)",				L"bool (::X<char, bool, float, double> ::) * * $PR"							);
	AssertExpr(pa, L"&_C4",			L"(& _C4)",				L"float __cdecl() * * $PR"													);
	AssertExpr(pa, L"&_D4",			L"(& _D4)",				L"double __thiscall() (::X<char, bool, float, double> ::) * * $PR"			);
	AssertExpr(pa, L"&_E4",			L"(& _E4)",				L"void * __cdecl() * * $PR"													);

	AssertExpr(pa, L"_A5",			L"_A5",					L"char * const & $L"														);
	AssertExpr(pa, L"_B5",			L"_B5",					L"bool (::X<char, bool, float, double> ::) * const & $L"					);
	AssertExpr(pa, L"_C5",			L"_C5",					L"float __cdecl() * const & $L"												);
	AssertExpr(pa, L"_D5",			L"_D5",					L"double __thiscall() (::X<char, bool, float, double> ::) * const & $L"		);
	AssertExpr(pa, L"_E5",			L"_E5",					L"void * __cdecl() * const & $L"											);

	AssertExpr(pa, L"&_A5",			L"(& _A5)",				L"char * const * $PR"														);
	AssertExpr(pa, L"&_B5",			L"(& _B5)",				L"bool (::X<char, bool, float, double> ::) * const * $PR"					);
	AssertExpr(pa, L"&_C5",			L"(& _C5)",				L"float __cdecl() * const * $PR"											);
	AssertExpr(pa, L"&_D5",			L"(& _D5)",				L"double __thiscall() (::X<char, bool, float, double> ::) * const * $PR"	);
	AssertExpr(pa, L"&_E5",			L"(& _E5)",				L"void * __cdecl() * const * $PR"											);

	AssertExpr(pa, L"_A6",			L"_A6",					L"char * const & $L"														);
	AssertExpr(pa, L"_B6",			L"_B6",					L"bool (::X<char, bool, float, double> ::) * const & $L"					);
	AssertExpr(pa, L"_C6",			L"_C6",					L"float __cdecl() * const & $L"												);
	AssertExpr(pa, L"_D6",			L"_D6",					L"double __thiscall() (::X<char, bool, float, double> ::) * const & $L"		);
	AssertExpr(pa, L"_E6",			L"_E6",					L"void * __cdecl() * const & $L"											);

	AssertExpr(pa, L"&_A6",			L"(& _A6)",				L"char * const * $PR"														);
	AssertExpr(pa, L"&_B6",			L"(& _B6)",				L"bool (::X<char, bool, float, double> ::) * const * $PR"					);
	AssertExpr(pa, L"&_C6",			L"(& _C6)",				L"float __cdecl() * const * $PR"											);
	AssertExpr(pa, L"&_D6",			L"(& _D6)",				L"double __thiscall() (::X<char, bool, float, double> ::) * const * $PR"	);
	AssertExpr(pa, L"&_E6",			L"(& _E6)",				L"void * __cdecl() * const * $PR"											);
}

TEST_CASE(TestParseGenericMember_AddressOfMember_OfExplicitOrImplicitThisExpr)
{
	auto input = LR"(
template<typename X>
struct Field{};

template<typename T>
struct S
{
	Field<T> f;
	Field<T>& r;
	const Field<T> c;
	static Field<T> s;

	template<typename U> void M1(U p)					{ using _S = S<T>; }
	template<typename U> void C1(U p)const				{ using _S = S<T>; }
	template<typename U> void V1(U p)volatile			{ using _S = S<T>; }
	template<typename U> void CV1(U p)const volatile	{ using _S = S<T>; }
	template<typename U> static void F1(U p)			{ using _S = S<T>; }

	template<typename U>void M2(U p);
	template<typename U>void C2(U p)const;
	template<typename U>void V2(U p)volatile;
	template<typename U>void CV2(U p)const volatile;
	template<typename U>static void F2(U p);
};

template<typename A>	template<typename B>	void S<A>::M2(B p)					{ using _S = S<A>; }
template<typename A>	template<typename B>	void S<A>::C2(B p)const				{ using _S = S<A>; }
template<typename A>	template<typename B>	void S<A>::V2(B p)volatile			{ using _S = S<A>; }
template<typename A>	template<typename B>	void S<A>::CV2(B p)const volatile	{ using _S = S<A>; }
template<typename A>	template<typename B>	void S<A>::F2(B p)					{ using _S = S<A>; }
)";
	COMPILE_PROGRAM(program, pa, input);
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"S", L"M" + itow(i)));
		WString classArg = i == 1 ? L"::S::[T]" : L"::S::M2::[A]";
		WString funcArg = L"::S::M" + itow(i) + L"::[" + (i == 1 ? L"U" : L"B") + L"]";
		WString fieldArg = L"::Field<" + classArg + L">";

		AssertExpr(spa, L"this",					L"this",						(L"::S<" + classArg + L"> * $PR").Buffer()							);
		AssertExpr(spa, L"p",						L"p",							(funcArg + L" $L").Buffer()											);

		AssertExpr(spa, L"f",						L"f",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::f",					L"_S :: f",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&f",						L"(& f)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::f",					L"(& _S :: f)",					(fieldArg + L" (::S<" + classArg + L"> ::) * $PR").Buffer()			);
		AssertExpr(spa, L"this->f",					L"this->f",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::f",				L"this->_S :: f",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::f",			L"(& this->_S :: f)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"r",						L"r",							(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"_S::r",					L"_S :: r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&r",						L"(& r)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::r",					L"(& _S :: r)",					(fieldArg + L" & (::S<" + classArg + L"> ::) * $PR").Buffer()		);
		AssertExpr(spa, L"this->r",					L"this->r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"this->_S::r",				L"this->_S :: r",				(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::r",			L"(& this->_S :: r)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"c",						L"c",							(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"_S::c",					L"_S :: c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&c",						L"(& c)",						(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&_S::c",					L"(& _S :: c)",					(fieldArg + L" const (::S<" + classArg + L"> ::) * $PR").Buffer()	);
		AssertExpr(spa, L"this->c",					L"this->c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"this->_S::c",				L"this->_S :: c",				(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&this->_S::c",			L"(& this->_S :: c)",			(fieldArg + L" const * $PR").Buffer()								);

		AssertExpr(spa, L"s",						L"s",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::s",					L"_S :: s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&s",						L"(& s)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::s",					L"(& _S :: s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"this->s",					L"this->s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::s",				L"this->_S :: s",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::s",			L"(& this->_S :: s)",			(fieldArg + L" * $PR").Buffer()										);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"S", L"C" + itow(i)));
		WString classArg = i == 1 ? L"::S::[T]" : L"::S::C2::[A]";
		WString funcArg = L"::S::C" + itow(i) + L"::[" + (i == 1 ? L"U" : L"B") + L"]";
		WString fieldArg = L"::Field<" + classArg + L">";

		AssertExpr(spa, L"this",					L"this",						(L"::S<" + classArg + L"> const * $PR").Buffer()					);
		AssertExpr(spa, L"p",						L"p",							(funcArg + L" $L").Buffer()											);

		AssertExpr(spa, L"f",						L"f",							(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"_S::f",					L"_S :: f",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&f",						L"(& f)",						(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&_S::f",					L"(& _S :: f)",					(fieldArg + L" (::S<" + classArg + L"> ::) * $PR").Buffer()			);
		AssertExpr(spa, L"this->f",					L"this->f",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"this->_S::f",				L"this->_S :: f",				(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&this->_S::f",			L"(& this->_S :: f)",			(fieldArg + L" const * $PR").Buffer()								);

		AssertExpr(spa, L"r",						L"r",							(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"_S::r",					L"_S :: r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&r",						L"(& r)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::r",					L"(& _S :: r)",					(fieldArg + L" & (::S<" + classArg + L"> ::) * $PR").Buffer()		);
		AssertExpr(spa, L"this->r",					L"this->r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"this->_S::r",				L"this->_S :: r",				(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::r",			L"(& this->_S :: r)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"c",						L"c",							(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"_S::c",					L"_S :: c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&c",						L"(& c)",						(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&_S::c",					L"(& _S :: c)",					(fieldArg + L" const (::S<" + classArg + L"> ::) * $PR").Buffer()	);
		AssertExpr(spa, L"this->c",					L"this->c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"this->_S::c",				L"this->_S :: c",				(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&this->_S::c",			L"(& this->_S :: c)",			(fieldArg + L" const * $PR").Buffer()								);

		AssertExpr(spa, L"s",						L"s",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::s",					L"_S :: s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&s",						L"(& s)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::s",					L"(& _S :: s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"this->s",					L"this->s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::s",				L"this->_S :: s",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::s",			L"(& this->_S :: s)",			(fieldArg + L" * $PR").Buffer()										);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"S", L"V" + itow(i)));
		WString classArg = i == 1 ? L"::S::[T]" : L"::S::V2::[A]";
		WString funcArg = L"::S::V" + itow(i) + L"::[" + (i == 1 ? L"U" : L"B") + L"]";
		WString fieldArg = L"::Field<" + classArg + L">";

		AssertExpr(spa, L"this",					L"this",						(L"::S<" + classArg + L"> volatile * $PR").Buffer()					);
		AssertExpr(spa, L"p",						L"p",							(funcArg + L" $L").Buffer()											);

		AssertExpr(spa, L"f",						L"f",							(fieldArg + L" volatile $L").Buffer()								);
		AssertExpr(spa, L"_S::f",					L"_S :: f",						(fieldArg + L" volatile $L").Buffer()								);
		AssertExpr(spa, L"&f",						L"(& f)",						(fieldArg + L" volatile * $PR").Buffer()							);
		AssertExpr(spa, L"&_S::f",					L"(& _S :: f)",					(fieldArg + L" (::S<" + classArg + L"> ::) * $PR").Buffer()			);
		AssertExpr(spa, L"this->f",					L"this->f",						(fieldArg + L" volatile $L").Buffer()								);
		AssertExpr(spa, L"this->_S::f",				L"this->_S :: f",				(fieldArg + L" volatile $L").Buffer()								);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					(fieldArg + L" volatile * $PR").Buffer()							);
		AssertExpr(spa, L"&this->_S::f",			L"(& this->_S :: f)",			(fieldArg + L" volatile * $PR").Buffer()							);

		AssertExpr(spa, L"r",						L"r",							(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"_S::r",					L"_S :: r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&r",						L"(& r)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::r",					L"(& _S :: r)",					(fieldArg + L" & (::S<" + classArg + L"> ::) * $PR").Buffer()		);
		AssertExpr(spa, L"this->r",					L"this->r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"this->_S::r",				L"this->_S :: r",				(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::r",			L"(& this->_S :: r)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"c",						L"c",							(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"_S::c",					L"_S :: c",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&c",						L"(& c)",						(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&_S::c",					L"(& _S :: c)",					(fieldArg + L" const (::S<" + classArg + L"> ::) * $PR").Buffer()	);
		AssertExpr(spa, L"this->c",					L"this->c",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"this->_S::c",				L"this->_S :: c",				(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&this->_S::c",			L"(& this->_S :: c)",			(fieldArg + L" const volatile * $PR").Buffer()						);

		AssertExpr(spa, L"s",						L"s",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::s",					L"_S :: s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&s",						L"(& s)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::s",					L"(& _S :: s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"this->s",					L"this->s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::s",				L"this->_S :: s",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::s",			L"(& this->_S :: s)",			(fieldArg + L" * $PR").Buffer()										);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"S", L"CV" + itow(i)));
		WString classArg = i == 1 ? L"::S::[T]" : L"::S::CV2::[A]";
		WString funcArg = L"::S::CV" + itow(i) + L"::[" + (i == 1 ? L"U" : L"B") + L"]";
		WString fieldArg = L"::Field<" + classArg + L">";

		AssertExpr(spa, L"this",					L"this",						(L"::S<" + classArg + L"> const volatile * $PR").Buffer()			);
		AssertExpr(spa, L"p",						L"p",							(funcArg + L" $L").Buffer()											);

		AssertExpr(spa, L"f",						L"f",							(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"_S::f",					L"_S :: f",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&f",						L"(& f)",						(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&_S::f",					L"(& _S :: f)",					(fieldArg + L" (::S<" + classArg + L"> ::) * $PR").Buffer()			);
		AssertExpr(spa, L"this->f",					L"this->f",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"this->_S::f",				L"this->_S :: f",				(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&this->_S::f",			L"(& this->_S :: f)",			(fieldArg + L" const volatile * $PR").Buffer()						);

		AssertExpr(spa, L"r",						L"r",							(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"_S::r",					L"_S :: r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&r",						L"(& r)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::r",					L"(& _S :: r)",					(fieldArg + L" & (::S<" + classArg + L"> ::) * $PR").Buffer()		);
		AssertExpr(spa, L"this->r",					L"this->r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"this->_S::r",				L"this->_S :: r",				(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::r",			L"(& this->_S :: r)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"c",						L"c",							(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"_S::c",					L"_S :: c",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&c",						L"(& c)",						(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&_S::c",					L"(& _S :: c)",					(fieldArg + L" const (::S<" + classArg + L"> ::) * $PR").Buffer()	);
		AssertExpr(spa, L"this->c",					L"this->c",						(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"this->_S::c",				L"this->_S :: c",				(fieldArg + L" const volatile $L").Buffer()							);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					(fieldArg + L" const volatile * $PR").Buffer()						);
		AssertExpr(spa, L"&this->_S::c",			L"(& this->_S :: c)",			(fieldArg + L" const volatile * $PR").Buffer()						);

		AssertExpr(spa, L"s",						L"s",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::s",					L"_S :: s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&s",						L"(& s)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::s",					L"(& _S :: s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"this->s",					L"this->s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::s",				L"this->_S :: s",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::s",			L"(& this->_S :: s)",			(fieldArg + L" * $PR").Buffer()										);
	}
	for (vint i = 1; i <= 2; i++)
	{
		auto spa = pa.WithScope(TestParseGenericMember_InsideFunction(pa, L"S", L"F" + itow(i)));
		WString classArg = i == 1 ? L"::S::[T]" : L"::S::F2::[A]";
		WString funcArg = L"::S::F" + itow(i) + L"::[" + (i == 1 ? L"U" : L"B") + L"]";
		WString fieldArg = L"::Field<" + classArg + L">";

		AssertExpr(spa, L"this",					L"this",						(L"::S<" + classArg + L"> * $PR").Buffer()							);
		AssertExpr(spa, L"p",						L"p",							(funcArg + L" $L").Buffer()											);

		AssertExpr(spa, L"f",						L"f",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::f",					L"_S :: f",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&f",						L"(& f)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::f",					L"(& _S :: f)",					(fieldArg + L" (::S<" + classArg + L"> ::) * $PR").Buffer()			);
		AssertExpr(spa, L"this->f",					L"this->f",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::f",				L"this->_S :: f",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->f",				L"(& this->f)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::f",			L"(& this->_S :: f)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"r",						L"r",							(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"_S::r",					L"_S :: r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&r",						L"(& r)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::r",					L"(& _S :: r)",					(fieldArg + L" & (::S<" + classArg + L"> ::) * $PR").Buffer()		);
		AssertExpr(spa, L"this->r",					L"this->r",						(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"this->_S::r",				L"this->_S :: r",				(fieldArg + L" & $L").Buffer()										);
		AssertExpr(spa, L"&this->r",				L"(& this->r)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::r",			L"(& this->_S :: r)",			(fieldArg + L" * $PR").Buffer()										);

		AssertExpr(spa, L"c",						L"c",							(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"_S::c",					L"_S :: c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&c",						L"(& c)",						(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&_S::c",					L"(& _S :: c)",					(fieldArg + L" const (::S<" + classArg + L"> ::) * $PR").Buffer()	);
		AssertExpr(spa, L"this->c",					L"this->c",						(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"this->_S::c",				L"this->_S :: c",				(fieldArg + L" const $L").Buffer()									);
		AssertExpr(spa, L"&this->c",				L"(& this->c)",					(fieldArg + L" const * $PR").Buffer()								);
		AssertExpr(spa, L"&this->_S::c",			L"(& this->_S :: c)",			(fieldArg + L" const * $PR").Buffer()								);

		AssertExpr(spa, L"s",						L"s",							(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"_S::s",					L"_S :: s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&s",						L"(& s)",						(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&_S::s",					L"(& _S :: s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"this->s",					L"this->s",						(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"this->_S::s",				L"this->_S :: s",				(fieldArg + L" $L").Buffer()										);
		AssertExpr(spa, L"&this->s",				L"(& this->s)",					(fieldArg + L" * $PR").Buffer()										);
		AssertExpr(spa, L"&this->_S::s",			L"(& this->_S :: s)",			(fieldArg + L" * $PR").Buffer()										);
	}
}

TEST_CASE(TestParseGenericMember_FieldReference)
{
	auto input = LR"(
template<typename X, typename Y>
struct Field{};

template<typename T, typename U>
struct A
{
	Field<T> a;
	Field<T> A<T, U>::* b;
	Field<U> C();
	Field<U> (A<T, U>::*d)();
	Field<U> E(...)const;
	Field<U> (A<T, U>::*f)(...)const;
};

A<int, double> a;
A<int, double>* pa;
const A<int, double> ca;
volatile A<int, double>* pva;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"a.a",									L"a.a",									L"::Field<__int32> $L"														);
	AssertExpr(pa, L"a.b",									L"a.b",									L"::Field<__int32> (::A<__int32, double> ::) * $L"							);
	AssertExpr(pa, L"a.C",									L"a.C",									L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"a.d",									L"a.d",									L"::Field<double> __thiscall() (::A<__int32, double> ::) * $L"				);
	AssertExpr(pa, L"a.E",									L"a.E",									L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"a.f",									L"a.f",									L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * $L"				);

	AssertExpr(pa, L"pa->a",								L"pa->a",								L"::Field<__int32> $L"														);
	AssertExpr(pa, L"pa->b",								L"pa->b",								L"::Field<__int32> (::A<__int32, double> ::) * $L"							);
	AssertExpr(pa, L"pa->C",								L"pa->C",								L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"pa->d",								L"pa->d",								L"::Field<double> __thiscall() (::A<__int32, double> ::) * $L"				);
	AssertExpr(pa, L"pa->E",								L"pa->E",								L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"pa->f",								L"pa->f",								L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * $L"				);

	AssertExpr(pa, L"ca.a",									L"ca.a",								L"::Field<__int32> const $L"												);
	AssertExpr(pa, L"ca.b",									L"ca.b",								L"::Field<__int32> (::A<__int32, double> ::) * const $L"					);
	AssertExpr(pa, L"ca.C",									L"ca.C"																												);
	AssertExpr(pa, L"ca.d",									L"ca.d",								L"::Field<double> __thiscall() (::A<__int32, double> ::) * const $L"		);
	AssertExpr(pa, L"ca.E",									L"ca.E",								L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"ca.f",									L"ca.f",								L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * const $L"		);

	AssertExpr(pa, L"pva->a",								L"pva->a",								L"::Field<__int32> volatile $L"												);
	AssertExpr(pa, L"pva->b",								L"pva->b",								L"::Field<__int32> (::A<__int32, double> ::) * volatile $L"					);
	AssertExpr(pa, L"pva->C",								L"pva->C"																											);
	AssertExpr(pa, L"pva->d",								L"pva->d",								L"::Field<double> __thiscall() (::A<__int32, double> ::) * volatile $L"		);
	AssertExpr(pa, L"pva->E",								L"pva->E"																											);
	AssertExpr(pa, L"pva->f",								L"pva->f",								L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * volatile $L"		);

	AssertExpr(pa, L"a.*&A<int,double>::a",					L"(a .* (& A<int, double> :: a))",		L"::Field<__int32> & $L"													);
	AssertExpr(pa, L"a.*&A<int,double>::b",					L"(a .* (& A<int, double> :: b))",		L"::Field<__int32> (::A<__int32, double> ::) * & $L"						);
	AssertExpr(pa, L"a.*&A<int,double>::C",					L"(a .* (& A<int, double> :: C))",		L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"a.*&A<int,double>::d",					L"(a .* (& A<int, double> :: d))",		L"::Field<double> __thiscall() (::A<__int32, double> ::) * & $L"			);
	AssertExpr(pa, L"a.*&A<int,double>::E",					L"(a .* (& A<int, double> :: E))",		L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"a.*&A<int,double>::f",					L"(a .* (& A<int, double> :: f))",		L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * & $L"			);

	AssertExpr(pa, L"pa->*&A<int,double>::a",				L"(pa ->* (& A<int, double> :: a))",	L"::Field<__int32> & $L"													);
	AssertExpr(pa, L"pa->*&A<int,double>::b",				L"(pa ->* (& A<int, double> :: b))",	L"::Field<__int32> (::A<__int32, double> ::) * & $L"						);
	AssertExpr(pa, L"pa->*&A<int,double>::C",				L"(pa ->* (& A<int, double> :: C))",	L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"pa->*&A<int,double>::d",				L"(pa ->* (& A<int, double> :: d))",	L"::Field<double> __thiscall() (::A<__int32, double> ::) * & $L"			);
	AssertExpr(pa, L"pa->*&A<int,double>::E",				L"(pa ->* (& A<int, double> :: E))",	L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"pa->*&A<int,double>::f",				L"(pa ->* (& A<int, double> :: f))",	L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * & $L"			);

	AssertExpr(pa, L"ca.*&A<int,double>::a",				L"(ca .* (& A<int, double> :: a))",		L"::Field<__int32> const & $L"												);
	AssertExpr(pa, L"ca.*&A<int,double>::b",				L"(ca .* (& A<int, double> :: b))",		L"::Field<__int32> (::A<__int32, double> ::) * const & $L"					);
	AssertExpr(pa, L"ca.*&A<int,double>::C",				L"(ca .* (& A<int, double> :: C))",		L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"ca.*&A<int,double>::d",				L"(ca .* (& A<int, double> :: d))",		L"::Field<double> __thiscall() (::A<__int32, double> ::) * const & $L"		);
	AssertExpr(pa, L"ca.*&A<int,double>::E",				L"(ca .* (& A<int, double> :: E))",		L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"ca.*&A<int,double>::f",				L"(ca .* (& A<int, double> :: f))",		L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * const & $L"		);

	AssertExpr(pa, L"pva->*&A<int,double>::a",				L"(pva ->* (& A<int, double> :: a))",	L"::Field<__int32> volatile & $L"											);
	AssertExpr(pa, L"pva->*&A<int,double>::b",				L"(pva ->* (& A<int, double> :: b))",	L"::Field<__int32> (::A<__int32, double> ::) * volatile & $L"				);
	AssertExpr(pa, L"pva->*&A<int,double>::C",				L"(pva ->* (& A<int, double> :: C))",	L"::Field<double> __thiscall() * $PR"										);
	AssertExpr(pa, L"pva->*&A<int,double>::d",				L"(pva ->* (& A<int, double> :: d))",	L"::Field<double> __thiscall() (::A<__int32, double> ::) * volatile & $L"	);
	AssertExpr(pa, L"pva->*&A<int,double>::E",				L"(pva ->* (& A<int, double> :: E))",	L"::Field<double> __cdecl(...) * $PR"										);
	AssertExpr(pa, L"pva->*&A<int,double>::f",				L"(pva ->* (& A<int, double> :: f))",	L"::Field<double> __cdecl(...) (::A<__int32, double> ::) * volatile & $L"	);
}