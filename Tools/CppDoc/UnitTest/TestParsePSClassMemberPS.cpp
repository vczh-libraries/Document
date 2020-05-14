#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Fields / Functions / Arrays")
	{
		auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct X<Tx&, Ty*>
{
	Tx* (&x)[1];
	Ty* (&y)[1];
};

template<typename Tx, typename Ty>
struct Y
{
	Tx x;
	Ty y;

	X<Tx*, Ty&>* operator->();
};

template<typename Tx, typename Ty>
struct Y<const Tx, volatile Ty>
{
	Tx& x;
	Ty& y;

	X<Tx&, Ty*>* operator->();
};

template<typename Tx, typename Ty>
struct Z
{
};

template<typename Tx, typename Ty>
struct Z<Tx*, Ty*>
{
	Tx** x;
	Ty** y;

	Y<Tx* const, Ty* volatile> operator->();
	X<Tx* const, Ty* volatile> operator()(int);
	Y<Tx* const, Ty* volatile> operator()(void*);
	X<Tx* const, Ty* volatile> operator[](const char*);
	Y<Tx* const, Ty* volatile> operator[](Z<Tx*, Ty*>&&);

	static Tx F(double);
	Ty G(void*);
};

Z<void*, char*> z;
Z<void*, char*>* pz = nullptr;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"pz->x",									L"pz->x",										L"void * * $L"													);
		AssertExpr(pa, L"pz->y",									L"pz->y",										L"char * * $L"													);
		AssertExpr(pa, L"pz->F",									L"pz->F",										L"void __cdecl(double) * $PR"									);
		AssertExpr(pa, L"pz->G",									L"pz->G",										L"char __thiscall(void *) * $PR"								);
		AssertExpr(pa, L"pz->operator->",							L"pz->operator ->",								L"::Y<void * const, char * volatile> __thiscall() * $PR"		);

		AssertExpr(pa, L"z.x",										L"z.x",											L"void * * $L"													);
		AssertExpr(pa, L"z.y",										L"z.y",											L"char * * $L"													);
		AssertExpr(pa, L"z.F",										L"z.F",											L"void __cdecl(double) * $PR"									);
		AssertExpr(pa, L"z.G",										L"z.G",											L"char __thiscall(void *) * $PR"								);
		AssertExpr(pa, L"z.operator->",								L"z.operator ->",								L"::Y<void * const, char * volatile> __thiscall() * $PR"		);

		AssertExpr(pa, L"pz->operator->()",							L"pz->operator ->()",							L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"pz->operator()(0)",						L"pz->operator ()(0)",							L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"pz->operator()(nullptr)",					L"pz->operator ()(nullptr)",					L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"pz->operator[](\"a\")",					L"pz->operator [](\"a\")",						L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"pz->operator[](Z<void*, char*>())",		L"pz->operator [](Z<void *, char *>())",		L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"pz->F(0)",									L"pz->F(0)",									L"void $PR"														);
		AssertExpr(pa, L"pz->G(0)",									L"pz->G(0)",									L"char $PR"														);

		AssertExpr(pa, L"z.operator->()",							L"z.operator ->()",								L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z.operator()(0)",							L"z.operator ()(0)",							L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z.operator()(nullptr)",					L"z.operator ()(nullptr)",						L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z.operator[](\"a\")",						L"z.operator [](\"a\")",						L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z.operator[](Z<void*, char*>())",			L"z.operator [](Z<void *, char *>())",			L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z.F(0)",									L"z.F(0)",										L"void $PR"														);
		AssertExpr(pa, L"z.G(0)",									L"z.G(0)",										L"char $PR"														);

		AssertExpr(pa, L"z->x",										L"z->x",										L"void * * [] & $L"													);
		AssertExpr(pa, L"z->y",										L"z->y",										L"char * * [] & $L"													);
		AssertExpr(pa, L"z(0)",										L"z(0)",										L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z(nullptr)",								L"z(nullptr)",									L"::Y<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z[\"a\"]",									L"z[\"a\"]",									L"::X<void * const, char * volatile> $PR"						);
		AssertExpr(pa, L"z[Z<void*, char*>()]",						L"z[Z<void *, char *>()]",						L"::Y<void * const, char * volatile> $PR"						);
	});

	TEST_CATEGORY(L"Fields from qualifiers")
	{
		auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<>
struct X<int, double>
{
	double x;
	int y;
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

		AssertExpr(pa, L"x.x",						L"x.x",							L"double $L"								);
		AssertExpr(pa, L"lx.x",						L"lx.x",						L"double $L"								);
		AssertExpr(pa, L"rx.x",						L"rx.x",						L"double $L"								);
		AssertExpr(pa, L"cx.x",						L"cx.x",						L"double const $L"							);
		AssertExpr(pa, L"clx.x",					L"clx.x",						L"double const $L"							);
		AssertExpr(pa, L"crx.x",					L"crx.x",						L"double const $L"							);

		AssertExpr(pa, L"x.y",						L"x.y",							L"__int32 $L"								);
		AssertExpr(pa, L"lx.y",						L"lx.y",						L"__int32 $L"								);
		AssertExpr(pa, L"rx.y",						L"rx.y",						L"__int32 $L"								);
		AssertExpr(pa, L"cx.y",						L"cx.y",						L"__int32 const $L"							);
		AssertExpr(pa, L"clx.y",					L"clx.y",						L"__int32 const $L"							);
		AssertExpr(pa, L"crx.y",					L"crx.y",						L"__int32 const $L"							);

		AssertExpr(pa, L"F().x",					L"F().x",						L"double $PR"								);
		AssertExpr(pa, L"lF().x",					L"lF().x",						L"double $L"								);
		AssertExpr(pa, L"rF().x",					L"rF().x",						L"double && $X"								);
		AssertExpr(pa, L"cF().x",					L"cF().x",						L"double const $PR"							);
		AssertExpr(pa, L"clF().x",					L"clF().x",						L"double const $L"							);
		AssertExpr(pa, L"crF().x",					L"crF().x",						L"double const && $X"						);
		AssertExpr(pa, L"px->x",					L"px->x",						L"double $L"								);
		AssertExpr(pa, L"cpx->x",					L"cpx->x",						L"double const $L"							);

		AssertExpr(pa, L"F().y",					L"F().y",						L"__int32 $PR"								);
		AssertExpr(pa, L"lF().y",					L"lF().y",						L"__int32 $L"								);
		AssertExpr(pa, L"rF().y",					L"rF().y",						L"__int32 && $X"							);
		AssertExpr(pa, L"cF().y",					L"cF().y",						L"__int32 const $PR"						);
		AssertExpr(pa, L"clF().y",					L"clF().y",						L"__int32 const $L"							);
		AssertExpr(pa, L"crF().y",					L"crF().y",						L"__int32 const && $X"						);
		AssertExpr(pa, L"px->y",					L"px->y",						L"__int32 $L"								);
		AssertExpr(pa, L"cpx->y",					L"cpx->y",						L"__int32 const $L"							);
	});

	TEST_CATEGORY(L"Qualifiers")
	{
		auto input = LR"(
template<typename Tx, typename Ty>
struct X
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct X<volatile Tx, const Ty>
{
	Tx x;
	Ty y;
};

template<typename Tx, typename Ty>
struct X<const Tx, volatile Ty>
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
		AssertExpr(pa, L"z->x",						L"z->x",						L"__int32 volatile $L"										);
		AssertExpr(pa, L"z(0)",						L"z(0)",						L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"z[0]",						L"z[0]",						L"::X<__int32 const, double volatile> $PR"					);

		AssertExpr(pa, L"cz.operator->()",			L"cz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
		AssertExpr(pa, L"cz.operator()(0)",			L"cz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"cz.operator[](0)",			L"cz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"cz->x",					L"cz->x",						L"__int32 $L"												);
		AssertExpr(pa, L"cz(0)",					L"cz(0)",						L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"cz[0]",					L"cz[0]",						L"::X<__int32 volatile, double const> $PR"					);

		AssertExpr(pa, L"lz.operator->()",			L"lz.operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
		AssertExpr(pa, L"lz.operator()(0)",			L"lz.operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"lz.operator[](0)",			L"lz.operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"lz->x",					L"lz->x",						L"__int32 volatile $L"										);
		AssertExpr(pa, L"lz(0)",					L"lz(0)",						L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"lz[0]",					L"lz[0]",						L"::X<__int32 const, double volatile> $PR"					);

		AssertExpr(pa, L"lcz.operator->()",			L"lcz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
		AssertExpr(pa, L"lcz.operator()(0)",		L"lcz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"lcz.operator[](0)",		L"lcz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"lcz->x",					L"lcz->x",						L"__int32 $L"												);
		AssertExpr(pa, L"lcz(0)",					L"lcz(0)",						L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"lcz[0]",					L"lcz[0]",						L"::X<__int32 volatile, double const> $PR"					);

		AssertExpr(pa, L"rz.operator->()",			L"rz.operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
		AssertExpr(pa, L"rz.operator()(0)",			L"rz.operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"rz.operator[](0)",			L"rz.operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"rz->x",					L"rz->x",						L"__int32 volatile $L"										);
		AssertExpr(pa, L"rz(0)",					L"rz(0)",						L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"rz[0]",					L"rz[0]",						L"::X<__int32 const, double volatile> $PR"					);

		AssertExpr(pa, L"rcz.operator->()",			L"rcz.operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
		AssertExpr(pa, L"rcz.operator()(0)",		L"rcz.operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"rcz.operator[](0)",		L"rcz.operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"rcz->x",					L"rcz->x",						L"__int32 $L"												);
		AssertExpr(pa, L"rcz(0)",					L"rcz(0)",						L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"rcz[0]",					L"rcz[0]",						L"::X<__int32 volatile, double const> $PR"					);

		AssertExpr(pa, L"pz->operator->()",			L"pz->operator ->()",			L"::X<__int32 const, double volatile> volatile * $PR"		);
		AssertExpr(pa, L"pz->operator()(0)",		L"pz->operator ()(0)",			L"::X<__int32 const, double volatile> $PR"					);
		AssertExpr(pa, L"pz->operator[](0)",		L"pz->operator [](0)",			L"::X<__int32 const, double volatile> $PR"					);

		AssertExpr(pa, L"pcz->operator->()",		L"pcz->operator ->()",			L"::X<__int32 volatile, double const> * $PR"				);
		AssertExpr(pa, L"pcz->operator()(0)",		L"pcz->operator ()(0)",			L"::X<__int32 volatile, double const> $PR"					);
		AssertExpr(pa, L"pcz->operator[](0)",		L"pcz->operator [](0)",			L"::X<__int32 volatile, double const> $PR"					);
	});

	TEST_CATEGORY(L"Qualifiers and this")
	{
	});

	TEST_CATEGORY(L"Address of members")
	{
	});

	TEST_CATEGORY(L"Address of members and this")
	{
	});

	TEST_CATEGORY(L"Referencing members")
	{
	});

	TEST_CATEGORY(L"Referencing generic members")
	{
	});

	TEST_CATEGORY(L"Re-index")
	{
	});

	TEST_CATEGORY(L"Members in base classes")
	{
	});
}