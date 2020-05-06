#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Fields / Functions / Arrays")
	{
		auto input = LR"(
struct X
{
	int x;
	int y;
};
struct Y
{
	double x;
	double y;

	X* operator->();
};
struct Z
{
	bool x;
	bool y;

	Y operator->();
	X operator()(int);
	Y operator()(void*);
	X operator[](const char*);
	Y operator[](Z&&);

	static int F(double);
	int G(void*);
};

X (*x)(int) = nullptr;
Y (*const y)(int) = nullptr;

X (*(*x2)(void*))(int) = nullptr;
Y (*const (*const y2)(void*))(int) = nullptr;

Z z;
Z* pz = nullptr;
	)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"pz->x",					L"pz->x",						L"bool $L"								);
		AssertExpr(pa, L"pz->y",					L"pz->y",						L"bool $L"								);
		AssertExpr(pa, L"pz->F",					L"pz->F",						L"__int32 __cdecl(double) * $PR"		);
		AssertExpr(pa, L"pz->G",					L"pz->G",						L"__int32 __thiscall(void *) * $PR"		);
		AssertExpr(pa, L"pz->operator->",			L"pz->operator ->",				L"::Y __thiscall() * $PR"				);

		AssertExpr(pa, L"z.x",						L"z.x",							L"bool $L"								);
		AssertExpr(pa, L"z.y",						L"z.y",							L"bool $L"								);
		AssertExpr(pa, L"z.F",						L"z.F",							L"__int32 __cdecl(double) * $PR"		);
		AssertExpr(pa, L"z.G",						L"z.G",							L"__int32 __thiscall(void *) * $PR"		);
		AssertExpr(pa, L"z.operator->",				L"z.operator ->",				L"::Y __thiscall() * $PR"				);

		AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::Y $PR"								);
		AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::X $PR"								);
		AssertExpr(pa, L"pz->operator()(nullptr)",	L"pz->operator ()(nullptr)",	L"::Y $PR"								);
		AssertExpr(pa, L"pz->operator[](\"a\")",	L"pz->operator [](\"a\")",		L"::X $PR"								);
		AssertExpr(pa, L"pz->operator[](Z())",		L"pz->operator [](Z())",		L"::Y $PR"								);
		AssertExpr(pa, L"pz->F(0)",					L"pz->F(0)",					L"__int32 $PR"							);
		AssertExpr(pa, L"pz->G(0)",					L"pz->G(0)",					L"__int32 $PR"							);

		AssertExpr(pa, L"z.operator->()",			L"z.operator ->()",				L"::Y $PR"								);
		AssertExpr(pa, L"z.operator()(0)",			L"z.operator ()(0)",			L"::X $PR"								);
		AssertExpr(pa, L"z.operator()(nullptr)",	L"z.operator ()(nullptr)",		L"::Y $PR"								);
		AssertExpr(pa, L"z.operator[](\"a\")",		L"z.operator [](\"a\")",		L"::X $PR"								);
		AssertExpr(pa, L"z.operator[](Z())",		L"z.operator [](Z())",			L"::Y $PR"								);
		AssertExpr(pa, L"z.F(0)",					L"z.F(0)",						L"__int32 $PR"							);
		AssertExpr(pa, L"z.G(0)",					L"z.G(0)",						L"__int32 $PR"							);

		AssertExpr(pa, L"z->x",						L"z->x",						L"__int32 $L"							);
		AssertExpr(pa, L"z->y",						L"z->y",						L"__int32 $L"							);
		AssertExpr(pa, L"z(0)",						L"z(0)",						L"::X $PR"								);
		AssertExpr(pa, L"z(nullptr)",				L"z(nullptr)",					L"::Y $PR"								);
		AssertExpr(pa, L"z[\"a\"]",					L"z[\"a\"]",					L"::X $PR"								);
		AssertExpr(pa, L"z[Z()]",					L"z[Z()]",						L"::Y $PR"								);

		AssertExpr(pa, L"x(0)",						L"x(0)",						L"::X $PR"								);
		AssertExpr(pa, L"y(0)",						L"y(0)",						L"::Y $PR"								);
		AssertExpr(pa, L"x2(nullptr)(0)",			L"x2(nullptr)(0)",				L"::X $PR"								);
		AssertExpr(pa, L"y2(nullptr)(0)",			L"y2(nullptr)(0)",				L"::Y $PR"								);
	});

	TEST_CATEGORY(L"Field qualifiers")
	{
		auto input = LR"(
struct X
{
	int x;
	int y;
};

X x;
X& lx;
X&& rx;

const X cx;
const X& clx;
const X&& crx;

X F();
X& lF();
X&& rF();

const X cF();
const X& clF();
const X&& crF();

X* px;
const X* cpx;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"x",						L"x",							L"::X $L"					);
		AssertExpr(pa, L"lx",						L"lx",							L"::X & $L"					);
		AssertExpr(pa, L"rx",						L"rx",							L"::X && $L"				);
		AssertExpr(pa, L"cx",						L"cx",							L"::X const $L"				);
		AssertExpr(pa, L"clx",						L"clx",							L"::X const & $L"			);
		AssertExpr(pa, L"crx",						L"crx",							L"::X const && $L"			);

		AssertExpr(pa, L"x.x",						L"x.x",							L"__int32 $L"				);
		AssertExpr(pa, L"lx.x",						L"lx.x",						L"__int32 $L"				);
		AssertExpr(pa, L"rx.x",						L"rx.x",						L"__int32 $L"				);
		AssertExpr(pa, L"cx.x",						L"cx.x",						L"__int32 const $L"			);
		AssertExpr(pa, L"clx.x",					L"clx.x",						L"__int32 const $L"			);
		AssertExpr(pa, L"crx.x",					L"crx.x",						L"__int32 const $L"			);

		AssertExpr(pa, L"F().x",					L"F().x",						L"__int32 $PR"				);
		AssertExpr(pa, L"lF().x",					L"lF().x",						L"__int32 $L"				);
		AssertExpr(pa, L"rF().x",					L"rF().x",						L"__int32 && $X"			);
		AssertExpr(pa, L"cF().x",					L"cF().x",						L"__int32 const $PR"		);
		AssertExpr(pa, L"clF().x",					L"clF().x",						L"__int32 const $L"			);
		AssertExpr(pa, L"crF().x",					L"crF().x",						L"__int32 const && $X"		);

		AssertExpr(pa, L"px",						L"px",							L"::X * $L"					);
		AssertExpr(pa, L"cpx",						L"cpx",							L"::X const * $L"			);
		AssertExpr(pa, L"px->x",					L"px->x",						L"__int32 $L"				);
		AssertExpr(pa, L"cpx->x",					L"cpx->x",						L"__int32 const $L"			);
	});

	TEST_CATEGORY(L"Array qualifiers")
	{
		auto input = LR"(
int x[1];
int (&lx)[1];
int (&&rx)[1];

const int cx[1];
const int (&clx)[1];
const int (&&crx)[1];

int (&lF())[1];
int (&&rF())[1];

const int (&clF())[1];
const int (&&crF())[1];

int* px;
const int* cpx;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"x[0]",						L"x[0]",						L"__int32 & $L"				);
		AssertExpr(pa, L"lx[0]",					L"lx[0]",						L"__int32 & $L"				);
		AssertExpr(pa, L"rx[0]",					L"rx[0]",						L"__int32 & $L"				);
		AssertExpr(pa, L"cx[0]",					L"cx[0]",						L"__int32 const & $L"		);
		AssertExpr(pa, L"clx[0]",					L"clx[0]",						L"__int32 const & $L"		);
		AssertExpr(pa, L"crx[0]",					L"crx[0]",						L"__int32 const & $L"		);

		AssertExpr(pa, L"lF()[0]",					L"lF()[0]",						L"__int32 & $L"				);
		AssertExpr(pa, L"rF()[0]",					L"rF()[0]",						L"__int32 && $X"			);
		AssertExpr(pa, L"clF()[0]",					L"clF()[0]",					L"__int32 const & $L"		);
		AssertExpr(pa, L"crF()[0]",					L"crF()[0]",					L"__int32 const && $X"		);

		AssertExpr(pa, L"px[0]",					L"px[0]",						L"__int32 & $L"				);
		AssertExpr(pa, L"cpx[0]",					L"cpx[0]",						L"__int32 const & $L"		);
	});

	TEST_CATEGORY(L"Fields / Functions / Arrays + Qualifiers")
	{
		auto input = LR"(
struct X
{
	int x;
	int y;
};
struct Y
{
	double x;
	double y;
};
struct Z
{
	X* operator->()const;
	const Y* operator->();
	X operator()(int)const;
	Y operator()(int);
	X operator[](int)const;
	Y operator[](int);
};

Z z;
const Z cz;

Z& lz;
const Z& lcz;

Z&& rz;
const Z&& rcz;

Z* const pz;
const Z* const pcz;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"z.operator->()",			L"z.operator ->()",				L"::Y const * $PR"			);
		AssertExpr(pa, L"z.operator()(0)",			L"z.operator ()(0)",			L"::Y $PR"					);
		AssertExpr(pa, L"z.operator[](0)",			L"z.operator [](0)",			L"::Y $PR"					);
		AssertExpr(pa, L"z->x",						L"z->x",						L"double const $L"			);
		AssertExpr(pa, L"z(0)",						L"z(0)",						L"::Y $PR"					);
		AssertExpr(pa, L"z[0]",						L"z[0]",						L"::Y $PR"					);

		AssertExpr(pa, L"cz.operator->()",			L"cz.operator ->()",			L"::X * $PR"				);
		AssertExpr(pa, L"cz.operator()(0)",			L"cz.operator ()(0)",			L"::X $PR"					);
		AssertExpr(pa, L"cz.operator[](0)",			L"cz.operator [](0)",			L"::X $PR"					);
		AssertExpr(pa, L"cz->x",					L"cz->x",						L"__int32 $L"				);
		AssertExpr(pa, L"cz(0)",					L"cz(0)",						L"::X $PR"					);
		AssertExpr(pa, L"cz[0]",					L"cz[0]",						L"::X $PR"					);

		AssertExpr(pa, L"lz.operator->()",			L"lz.operator ->()",			L"::Y const * $PR"			);
		AssertExpr(pa, L"lz.operator()(0)",			L"lz.operator ()(0)",			L"::Y $PR"					);
		AssertExpr(pa, L"lz.operator[](0)",			L"lz.operator [](0)",			L"::Y $PR"					);
		AssertExpr(pa, L"lz->x",					L"lz->x",						L"double const $L"			);
		AssertExpr(pa, L"lz(0)",					L"lz(0)",						L"::Y $PR"					);
		AssertExpr(pa, L"lz[0]",					L"lz[0]",						L"::Y $PR"					);

		AssertExpr(pa, L"lcz.operator->()",			L"lcz.operator ->()",			L"::X * $PR"				);
		AssertExpr(pa, L"lcz.operator()(0)",		L"lcz.operator ()(0)",			L"::X $PR"					);
		AssertExpr(pa, L"lcz.operator[](0)",		L"lcz.operator [](0)",			L"::X $PR"					);
		AssertExpr(pa, L"lcz->x",					L"lcz->x",						L"__int32 $L"				);
		AssertExpr(pa, L"lcz(0)",					L"lcz(0)",						L"::X $PR"					);
		AssertExpr(pa, L"lcz[0]",					L"lcz[0]",						L"::X $PR"					);

		AssertExpr(pa, L"rz.operator->()",			L"rz.operator ->()",			L"::Y const * $PR"			);
		AssertExpr(pa, L"rz.operator()(0)",			L"rz.operator ()(0)",			L"::Y $PR"					);
		AssertExpr(pa, L"rz.operator[](0)",			L"rz.operator [](0)",			L"::Y $PR"					);
		AssertExpr(pa, L"rz->x",					L"rz->x",						L"double const $L"			);
		AssertExpr(pa, L"rz(0)",					L"rz(0)",						L"::Y $PR"					);
		AssertExpr(pa, L"rz[0]",					L"rz[0]",						L"::Y $PR"					);

		AssertExpr(pa, L"rcz.operator->()",			L"rcz.operator ->()",			L"::X * $PR"				);
		AssertExpr(pa, L"rcz.operator()(0)",		L"rcz.operator ()(0)",			L"::X $PR"					);
		AssertExpr(pa, L"rcz.operator[](0)",		L"rcz.operator [](0)",			L"::X $PR"					);
		AssertExpr(pa, L"rcz->x",					L"rcz->x",						L"__int32 $L"				);
		AssertExpr(pa, L"rcz(0)",					L"rcz(0)",						L"::X $PR"					);
		AssertExpr(pa, L"rcz[0]",					L"rcz[0]",						L"::X $PR"					);

		AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::Y const * $PR"			);
		AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::Y $PR"					);
		AssertExpr(pa, L"pz->operator[](0)",		L"pz->operator [](0)",			L"::Y $PR"					);

		AssertExpr(pa, L"pcz->operator->()",		L"pcz->operator ->()",			L"::X * $PR"				);
		AssertExpr(pa, L"pcz->operator()(0)",		L"pcz->operator ()(0)",			L"::X $PR"					);
		AssertExpr(pa, L"pcz->operator[](0)",		L"pcz->operator [](0)",			L"::X $PR"					);
	});

	auto TestParseMember_InsideFunction = [](const ParsingArguments& pa, const WString& className, const WString& methodName)
	{
		auto classSymbol = pa.scopeSymbol->TryGetChildren_NFb(className)->Get(0);
		auto functionSymbol = classSymbol->TryGetChildren_NFb(methodName)->Get(0);
		auto functionBodySymbol = functionSymbol->GetImplSymbols_F()[0];
		return functionBodySymbol->TryGetChildren_NFb(L"$")->Get(0).Obj();
	};

	TEST_CATEGORY(L"Qualifiers and this")
	{
		auto input = LR"(
struct X
{
	int x;
	int y;
};
struct Y
{
	double x;
	double y;
};
struct Z
{
	X* operator->()const;
	const Y* operator->();
	X operator()(int)const;
	Y operator()(int);
	X operator[](int)const;
	Y operator[](int);

	void M(){}
	void C()const{}
};
)";
		COMPILE_PROGRAM(program, pa, input);

		{
			auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"Z", L"M"));

			AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::Y const * $PR"			);
			AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::Y $PR"					);
			AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::Y $PR"					);
			AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"double const $L"			);
			AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::Y $PR"					);
			AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::Y $PR"					);
		}
		{
			auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"Z", L"C"));

			AssertExpr(spa, L"operator->()",			L"operator ->()",				L"::X * $PR"				);
			AssertExpr(spa, L"operator()(0)",			L"operator ()(0)",				L"::X $PR"					);
			AssertExpr(spa, L"operator[](0)",			L"operator [](0)",				L"::X $PR"					);
			AssertExpr(spa, L"(*this)->x",				L"((* this))->x",				L"__int32 $L"				);
			AssertExpr(spa, L"(*this)(0)",				L"((* this))(0)",				L"::X $PR"					);
			AssertExpr(spa, L"(*this)[0]",				L"((* this))[0]",				L"::X $PR"					);
		}
	});

	TEST_CATEGORY(L"Address of members")
	{
		auto input = LR"(
struct X
{
	static int A;
	int B;
	static int C();
	int D();
};

struct Y : X
{
};

int E();

auto			_A1 = &Y::A;
auto			_B1 = &Y::B;
auto			_C1 = &Y::C;
auto			_D1 = &Y::D;
auto			_E1 = &E;

decltype(auto)	_A2 = &Y::A;
decltype(auto)	_B2 = &Y::B;
decltype(auto)	_C2 = &Y::C;
decltype(auto)	_D2 = &Y::D;
decltype(auto)	_E2 = &E;

decltype(&Y::A)	_A3[1] = {&Y::A};
decltype(&Y::B)	_B3[1] = {&Y::B};
decltype(&Y::C)	_C3[1] = {&Y::C};
decltype(&Y::D)	_D3[1] = {&Y::D};
decltype(&E)	_E3[1] = {&E};

auto&&			_A4 = &Y::A;
auto&&			_B4 = &Y::B;
auto&&			_C4 = &Y::C;
auto&&			_D4 = &Y::D;
auto&&			_E4 = &E;

const auto&		_A5 = &Y::A;
const auto&		_B5 = &Y::B;
const auto&		_C5 = &Y::C;
const auto&		_D5 = &Y::D;
const auto&		_E5 = &E;

auto&			_A6 = _A5;
auto&			_B6 = _B5;
auto&			_C6 = _C5;
auto&			_D6 = _D5;
auto&			_E6 = _E5;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"Y::A",			L"Y :: A",				L"__int32 $L"									);
		AssertExpr(pa, L"Y::B",			L"Y :: B",				L"__int32 $L"									);
		AssertExpr(pa, L"Y::C",			L"Y :: C",				L"__int32 __cdecl() * $PR"						);
		AssertExpr(pa, L"Y::D",			L"Y :: D",				L"__int32 __thiscall() * $PR"					);
		AssertExpr(pa, L"E",			L"E",					L"__int32 __cdecl() * $PR"						);

		AssertExpr(pa, L"&Y::A",		L"(& Y :: A)",			L"__int32 * $PR"								);
		AssertExpr(pa, L"&Y::B",		L"(& Y :: B)",			L"__int32 (::X ::) * $PR"						);
		AssertExpr(pa, L"&Y::C",		L"(& Y :: C)",			L"__int32 __cdecl() * $PR"						);
		AssertExpr(pa, L"&Y::D",		L"(& Y :: D)",			L"__int32 __thiscall() (::X ::) * $PR"			);
		AssertExpr(pa, L"&E",			L"(& E)",				L"__int32 __cdecl() * $PR"						);

		AssertExpr(pa, L"_A1",			L"_A1",					L"__int32 * $L"									);
		AssertExpr(pa, L"_B1",			L"_B1",					L"__int32 (::X ::) * $L"						);
		AssertExpr(pa, L"_C1",			L"_C1",					L"__int32 __cdecl() * $L"						);
		AssertExpr(pa, L"_D1",			L"_D1",					L"__int32 __thiscall() (::X ::) * $L"			);
		AssertExpr(pa, L"_E1",			L"_E1",					L"__int32 __cdecl() * $L"						);

		AssertExpr(pa, L"&_A1",			L"(& _A1)",				L"__int32 * * $PR"								);
		AssertExpr(pa, L"&_B1",			L"(& _B1)",				L"__int32 (::X ::) * * $PR"						);
		AssertExpr(pa, L"&_C1",			L"(& _C1)",				L"__int32 __cdecl() * * $PR"					);
		AssertExpr(pa, L"&_D1",			L"(& _D1)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
		AssertExpr(pa, L"&_E1",			L"(& _E1)",				L"__int32 __cdecl() * * $PR"					);

		AssertExpr(pa, L"_A2",			L"_A2",					L"__int32 * $L"									);
		AssertExpr(pa, L"_B2",			L"_B2",					L"__int32 (::X ::) * $L"						);
		AssertExpr(pa, L"_C2",			L"_C2",					L"__int32 __cdecl() * $L"						);
		AssertExpr(pa, L"_D2",			L"_D2",					L"__int32 __thiscall() (::X ::) * $L"			);
		AssertExpr(pa, L"_E2",			L"_E2",					L"__int32 __cdecl() * $L"						);

		AssertExpr(pa, L"&_A2",			L"(& _A2)",				L"__int32 * * $PR"								);
		AssertExpr(pa, L"&_B2",			L"(& _B2)",				L"__int32 (::X ::) * * $PR"						);
		AssertExpr(pa, L"&_C2",			L"(& _C2)",				L"__int32 __cdecl() * * $PR"					);
		AssertExpr(pa, L"&_D2",			L"(& _D2)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
		AssertExpr(pa, L"&_E2",			L"(& _E2)",				L"__int32 __cdecl() * * $PR"					);

		AssertExpr(pa, L"_A3",			L"_A3",					L"__int32 * [] $L"								);
		AssertExpr(pa, L"_B3",			L"_B3",					L"__int32 (::X ::) * [] $L"						);
		AssertExpr(pa, L"_C3",			L"_C3",					L"__int32 __cdecl() * [] $L"					);
		AssertExpr(pa, L"_D3",			L"_D3",					L"__int32 __thiscall() (::X ::) * [] $L"		);
		AssertExpr(pa, L"_E3",			L"_E3",					L"__int32 __cdecl() * [] $L"					);

		AssertExpr(pa, L"&_A3",			L"(& _A3)",				L"__int32 * [] * $PR"							);
		AssertExpr(pa, L"&_B3",			L"(& _B3)",				L"__int32 (::X ::) * [] * $PR"					);
		AssertExpr(pa, L"&_C3",			L"(& _C3)",				L"__int32 __cdecl() * [] * $PR"					);
		AssertExpr(pa, L"&_D3",			L"(& _D3)",				L"__int32 __thiscall() (::X ::) * [] * $PR"		);
		AssertExpr(pa, L"&_E3",			L"(& _E3)",				L"__int32 __cdecl() * [] * $PR"					);

		AssertExpr(pa, L"_A4",			L"_A4",					L"__int32 * && $L"								);
		AssertExpr(pa, L"_B4",			L"_B4",					L"__int32 (::X ::) * && $L"						);
		AssertExpr(pa, L"_C4",			L"_C4",					L"__int32 __cdecl() * && $L"					);
		AssertExpr(pa, L"_D4",			L"_D4",					L"__int32 __thiscall() (::X ::) * && $L"		);
		AssertExpr(pa, L"_E4",			L"_E4",					L"__int32 __cdecl() * && $L"					);

		AssertExpr(pa, L"&_A4",			L"(& _A4)",				L"__int32 * * $PR"								);
		AssertExpr(pa, L"&_B4",			L"(& _B4)",				L"__int32 (::X ::) * * $PR"						);
		AssertExpr(pa, L"&_C4",			L"(& _C4)",				L"__int32 __cdecl() * * $PR"					);
		AssertExpr(pa, L"&_D4",			L"(& _D4)",				L"__int32 __thiscall() (::X ::) * * $PR"		);
		AssertExpr(pa, L"&_E4",			L"(& _E4)",				L"__int32 __cdecl() * * $PR"					);

		AssertExpr(pa, L"_A5",			L"_A5",					L"__int32 * const & $L"							);
		AssertExpr(pa, L"_B5",			L"_B5",					L"__int32 (::X ::) * const & $L"				);
		AssertExpr(pa, L"_C5",			L"_C5",					L"__int32 __cdecl() * const & $L"				);
		AssertExpr(pa, L"_D5",			L"_D5",					L"__int32 __thiscall() (::X ::) * const & $L"	);
		AssertExpr(pa, L"_E5",			L"_E5",					L"__int32 __cdecl() * const & $L"				);

		AssertExpr(pa, L"&_A5",			L"(& _A5)",				L"__int32 * const * $PR"						);
		AssertExpr(pa, L"&_B5",			L"(& _B5)",				L"__int32 (::X ::) * const * $PR"				);
		AssertExpr(pa, L"&_C5",			L"(& _C5)",				L"__int32 __cdecl() * const * $PR"				);
		AssertExpr(pa, L"&_D5",			L"(& _D5)",				L"__int32 __thiscall() (::X ::) * const * $PR"	);
		AssertExpr(pa, L"&_E5",			L"(& _E5)",				L"__int32 __cdecl() * const * $PR"				);

		AssertExpr(pa, L"_A6",			L"_A6",					L"__int32 * const & $L"							);
		AssertExpr(pa, L"_B6",			L"_B6",					L"__int32 (::X ::) * const & $L"				);
		AssertExpr(pa, L"_C6",			L"_C6",					L"__int32 __cdecl() * const & $L"				);
		AssertExpr(pa, L"_D6",			L"_D6",					L"__int32 __thiscall() (::X ::) * const & $L"	);
		AssertExpr(pa, L"_E6",			L"_E6",					L"__int32 __cdecl() * const & $L"				);

		AssertExpr(pa, L"&_A6",			L"(& _A6)",				L"__int32 * const * $PR"						);
		AssertExpr(pa, L"&_B6",			L"(& _B6)",				L"__int32 (::X ::) * const * $PR"				);
		AssertExpr(pa, L"&_C6",			L"(& _C6)",				L"__int32 __cdecl() * const * $PR"				);
		AssertExpr(pa, L"&_D6",			L"(& _D6)",				L"__int32 __thiscall() (::X ::) * const * $PR"	);
		AssertExpr(pa, L"&_E6",			L"(& _E6)",				L"__int32 __cdecl() * const * $PR"				);
	});

	TEST_CATEGORY(L"Address of members and this")
	{
		auto input = LR"(
struct S
{
	int f;
	int& r;
	const int c;
	static int s;

	void M1(double p){}
	void C1(double p)const{}
	void V1(double p)volatile{}
	void CV1(double p)const volatile{}
	static void F1(double p){}

	void M2(double p);
	void C2(double p)const;
	void V2(double p)volatile;
	void CV2(double p)const volatile;
	static void F2(double p);
};

void S::M2(double p){}
void S::C2(double p)const{}
void S::V2(double p)volatile{}
void S::CV2(double p)const volatile{}
void S::F2(double p){}
)";
		COMPILE_PROGRAM(program, pa, input);
		for (vint i = 1; i <= 2; i++)
		{
			TEST_CATEGORY(L"M" + itow(i))
			{
				auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"S", L"M" + itow(i)));

				AssertExpr(spa, L"this",					L"this",						L"::S * $PR"								);
				AssertExpr(spa, L"p",						L"p",							L"double $L"								);

				AssertExpr(spa, L"f",						L"f",							L"__int32 $L"								);
				AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 $L"								);
				AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
				AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
				AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
				AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
				AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
				AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
				AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
				AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
				AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
				AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
				AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
				AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
				AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
				AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

				AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
				AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
				AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
			});
		}
		for (vint i = 1; i <= 2; i++)
		{
			TEST_CATEGORY(L"C" + itow(i))
			{
				auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"S", L"C" + itow(i)));

				AssertExpr(spa, L"this",					L"this",						L"::S const * $PR"							);
				AssertExpr(spa, L"p",						L"p",							L"double $L"								);

				AssertExpr(spa, L"f",						L"f",							L"__int32 const $L"							);
				AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 const $L"							);
				AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 const * $PR"						);
				AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
				AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 const $L"							);
				AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 const $L"							);
				AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 const * $PR"						);
				AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 const * $PR"						);

				AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
				AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
				AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
				AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
				AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
				AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
				AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
				AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
				AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
				AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
				AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
				AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
				AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

				AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
				AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
				AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
			});
		}
		for (vint i = 1; i <= 2; i++)
		{
			TEST_CATEGORY(L"V" + itow(i))
			{
				auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"S", L"V" + itow(i)));

				AssertExpr(spa, L"this",					L"this",						L"::S volatile * $PR"						);
				AssertExpr(spa, L"p",						L"p",							L"double $L"								);

				AssertExpr(spa, L"f",						L"f",							L"__int32 volatile $L"						);
				AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 volatile $L"						);
				AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 volatile * $PR"					);
				AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
				AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 volatile $L"						);
				AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 volatile $L"						);
				AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 volatile * $PR"					);
				AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 volatile * $PR"					);

				AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
				AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
				AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
				AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
				AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
				AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"c",						L"c",							L"__int32 const volatile $L"				);
				AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
				AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const volatile * $PR"				);

				AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
				AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
				AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
			});
		}
		for (vint i = 1; i <= 2; i++)
		{
			TEST_CATEGORY(L"CV" + itow(i))
			{
				auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"S", L"CV" + itow(i)));

				AssertExpr(spa, L"this",					L"this",						L"::S const volatile * $PR"					);
				AssertExpr(spa, L"p",						L"p",							L"double $L"								);

				AssertExpr(spa, L"f",						L"f",							L"__int32 const volatile $L"				);
				AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
				AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 const volatile * $PR"				);

				AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
				AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
				AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
				AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
				AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
				AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"c",						L"c",							L"__int32 const volatile $L"				);
				AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
				AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const volatile $L"				);
				AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const volatile $L"				);
				AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const volatile * $PR"				);
				AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const volatile * $PR"				);

				AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
				AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
				AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
			});
		}
		for (vint i = 1; i <= 2; i++)
		{
			TEST_CATEGORY(L"F" + itow(i))
			{
				auto spa = pa.WithScope(TestParseMember_InsideFunction(pa, L"S", L"F" + itow(i)));

				AssertExpr(spa, L"this",					L"this",						L"::S * $PR"								);
				AssertExpr(spa, L"p",						L"p",							L"double $L"								);

				AssertExpr(spa, L"f",						L"f",							L"__int32 $L"								);
				AssertExpr(spa, L"S::f",					L"S :: f",						L"__int32 $L"								);
				AssertExpr(spa, L"&f",						L"(& f)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::f",					L"(& S :: f)",					L"__int32 (::S ::) * $PR"					);
				AssertExpr(spa, L"this->f",					L"this->f",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::f",				L"this->S :: f",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->f",				L"(& this->f)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::f",				L"(& this->S :: f)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"r",						L"r",							L"__int32 & $L"								);
				AssertExpr(spa, L"S::r",					L"S :: r",						L"__int32 & $L"								);
				AssertExpr(spa, L"&r",						L"(& r)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::r",					L"(& S :: r)",					L"__int32 & (::S ::) * $PR"					);
				AssertExpr(spa, L"this->r",					L"this->r",						L"__int32 & $L"								);
				AssertExpr(spa, L"this->S::r",				L"this->S :: r",				L"__int32 & $L"								);
				AssertExpr(spa, L"&this->r",				L"(& this->r)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::r",				L"(& this->S :: r)",			L"__int32 * $PR"							);

				AssertExpr(spa, L"c",						L"c",							L"__int32 const $L"							);
				AssertExpr(spa, L"S::c",					L"S :: c",						L"__int32 const $L"							);
				AssertExpr(spa, L"&c",						L"(& c)",						L"__int32 const * $PR"						);
				AssertExpr(spa, L"&S::c",					L"(& S :: c)",					L"__int32 const (::S ::) * $PR"				);
				AssertExpr(spa, L"this->c",					L"this->c",						L"__int32 const $L"							);
				AssertExpr(spa, L"this->S::c",				L"this->S :: c",				L"__int32 const $L"							);
				AssertExpr(spa, L"&this->c",				L"(& this->c)",					L"__int32 const * $PR"						);
				AssertExpr(spa, L"&this->S::c",				L"(& this->S :: c)",			L"__int32 const * $PR"						);

				AssertExpr(spa, L"s",						L"s",							L"__int32 $L"								);
				AssertExpr(spa, L"S::s",					L"S :: s",						L"__int32 $L"								);
				AssertExpr(spa, L"&s",						L"(& s)",						L"__int32 * $PR"							);
				AssertExpr(spa, L"&S::s",					L"(& S :: s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"this->s",					L"this->s",						L"__int32 $L"								);
				AssertExpr(spa, L"this->S::s",				L"this->S :: s",				L"__int32 $L"								);
				AssertExpr(spa, L"&this->s",				L"(& this->s)",					L"__int32 * $PR"							);
				AssertExpr(spa, L"&this->S::s",				L"(& this->S :: s)",			L"__int32 * $PR"							);
			});
		}
	});

	TEST_CATEGORY(L"Referencing fields")
	{
		auto input = LR"(
struct A
{
	int a;
	int A::* b;
	double C();
	double (A::*d)();
	double E(...)const;
	double (A::*f)(...)const;
};

A a;
A* pa;
const A ca;
volatile A* pva;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"a.a",						L"a.a",						L"__int32 $L"										);
		AssertExpr(pa, L"a.b",						L"a.b",						L"__int32 (::A ::) * $L"							);
		AssertExpr(pa, L"a.C",						L"a.C",						L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"a.d",						L"a.d",						L"double __thiscall() (::A ::) * $L"				);
		AssertExpr(pa, L"a.E",						L"a.E",						L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"a.f",						L"a.f",						L"double __cdecl(...) (::A ::) * $L"				);

		AssertExpr(pa, L"pa->a",					L"pa->a",					L"__int32 $L"										);
		AssertExpr(pa, L"pa->b",					L"pa->b",					L"__int32 (::A ::) * $L"							);
		AssertExpr(pa, L"pa->C",					L"pa->C",					L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"pa->d",					L"pa->d",					L"double __thiscall() (::A ::) * $L"				);
		AssertExpr(pa, L"pa->E",					L"pa->E",					L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"pa->f",					L"pa->f",					L"double __cdecl(...) (::A ::) * $L"				);

		AssertExpr(pa, L"ca.a",						L"ca.a",					L"__int32 const $L"									);
		AssertExpr(pa, L"ca.b",						L"ca.b",					L"__int32 (::A ::) * const $L"						);
		AssertExpr(pa, L"ca.C",						L"ca.C"																			);
		AssertExpr(pa, L"ca.d",						L"ca.d",					L"double __thiscall() (::A ::) * const $L"			);
		AssertExpr(pa, L"ca.E",						L"ca.E",					L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"ca.f",						L"ca.f",					L"double __cdecl(...) (::A ::) * const $L"			);

		AssertExpr(pa, L"pva->a",					L"pva->a",					L"__int32 volatile $L"								);
		AssertExpr(pa, L"pva->b",					L"pva->b",					L"__int32 (::A ::) * volatile $L"					);
		AssertExpr(pa, L"pva->C",					L"pva->C"																		);
		AssertExpr(pa, L"pva->d",					L"pva->d",					L"double __thiscall() (::A ::) * volatile $L"		);
		AssertExpr(pa, L"pva->E",					L"pva->E"																		);
		AssertExpr(pa, L"pva->f",					L"pva->f",					L"double __cdecl(...) (::A ::) * volatile $L"		);

		AssertExpr(pa, L"a.*&A::a",					L"(a .* (& A :: a))",		L"__int32 & $L"										);
		AssertExpr(pa, L"a.*&A::b",					L"(a .* (& A :: b))",		L"__int32 (::A ::) * & $L"							);
		AssertExpr(pa, L"a.*&A::C",					L"(a .* (& A :: C))",		L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"a.*&A::d",					L"(a .* (& A :: d))",		L"double __thiscall() (::A ::) * & $L"				);
		AssertExpr(pa, L"a.*&A::E",					L"(a .* (& A :: E))",		L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"a.*&A::f",					L"(a .* (& A :: f))",		L"double __cdecl(...) (::A ::) * & $L"				);

		AssertExpr(pa, L"pa->*&A::a",				L"(pa ->* (& A :: a))",		L"__int32 & $L"										);
		AssertExpr(pa, L"pa->*&A::b",				L"(pa ->* (& A :: b))",		L"__int32 (::A ::) * & $L"							);
		AssertExpr(pa, L"pa->*&A::C",				L"(pa ->* (& A :: C))",		L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"pa->*&A::d",				L"(pa ->* (& A :: d))",		L"double __thiscall() (::A ::) * & $L"				);
		AssertExpr(pa, L"pa->*&A::E",				L"(pa ->* (& A :: E))",		L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"pa->*&A::f",				L"(pa ->* (& A :: f))",		L"double __cdecl(...) (::A ::) * & $L"				);

		AssertExpr(pa, L"ca.*&A::a",				L"(ca .* (& A :: a))",		L"__int32 const & $L"								);
		AssertExpr(pa, L"ca.*&A::b",				L"(ca .* (& A :: b))",		L"__int32 (::A ::) * const & $L"					);
		AssertExpr(pa, L"ca.*&A::C",				L"(ca .* (& A :: C))",		L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"ca.*&A::d",				L"(ca .* (& A :: d))",		L"double __thiscall() (::A ::) * const & $L"		);
		AssertExpr(pa, L"ca.*&A::E",				L"(ca .* (& A :: E))",		L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"ca.*&A::f",				L"(ca .* (& A :: f))",		L"double __cdecl(...) (::A ::) * const & $L"		);

		AssertExpr(pa, L"pva->*&A::a",				L"(pva ->* (& A :: a))",	L"__int32 volatile & $L"							);
		AssertExpr(pa, L"pva->*&A::b",				L"(pva ->* (& A :: b))",	L"__int32 (::A ::) * volatile & $L"					);
		AssertExpr(pa, L"pva->*&A::C",				L"(pva ->* (& A :: C))",	L"double __thiscall() * $PR"						);
		AssertExpr(pa, L"pva->*&A::d",				L"(pva ->* (& A :: d))",	L"double __thiscall() (::A ::) * volatile & $L"		);
		AssertExpr(pa, L"pva->*&A::E",				L"(pva ->* (& A :: E))",	L"double __cdecl(...) * $PR"						);
		AssertExpr(pa, L"pva->*&A::f",				L"(pva ->* (& A :: f))",	L"double __cdecl(...) (::A ::) * volatile & $L"		);
	});
}