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
	});

	TEST_CATEGORY(L"Qualifiers")
	{
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