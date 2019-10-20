#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericClass)
{
	auto input = LR"(
template<typename T>
struct Ptr
{
private:
	T* pointer = nullptr;
public:
	Ptr() = default;
	Ptr(T* _pointer):pointer(_pointer){}
	~Ptr(){delete pointer;}
	operator T*()const{return pointer;}
	T* operator->()const{return pointer;}
};
)";

	auto output = LR"(
template<typename T>
struct Ptr
{
	private pointer: T * = nullptr;
	public __forward __ctor $__ctor: __null () = default;
	public __ctor $__ctor: __null (_pointer: T *)
		: pointer(_pointer)
	{
	}
	public __dtor ~Ptr: __null ()
	{
		delete (pointer);
	}
	public __type $__type: T * () const
	{
		return pointer;
	}
	public operator ->: T * () const
	{
		return pointer;
	}
};
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertType(pa,		L"Ptr",			L"Ptr",			L"<::Ptr::[T]> ::Ptr<::Ptr::[T]>"	);
	AssertType(pa,		L"Ptr<int>",	L"Ptr<int>",	L"::Ptr<__int32>"					);
}

TEST_CASE(TestParseGenericClass_NestedStructUsedOutside)
{
	auto input = LR"(
template<typename TA>
struct GA
{
	struct CB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};
	};

	template<typename TB>
	struct GB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};
	};
};
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(pa,		L"GA",								L"GA",									L"<::GA::[TA]> ::GA<::GA::[TA]>"										);
	AssertType(pa,		L"GA<int>::CB",						L"GA<int> :: CB",						L"::GA<__int32>::CB"													);
	AssertType(pa,		L"GA<int>::GB",						L"GA<int> :: GB",						L"<::GA::GB::[TB]> ::GA<__int32>::GB<::GA::GB::[TB]>"					);
	AssertType(pa,		L"GA<int>::GB<bool>",				L"GA<int> :: GB<bool>",					L"::GA<__int32>::GB<bool>"												);
	AssertType(pa,		L"GA<int>::CB::CC",					L"GA<int> :: CB :: CC",					L"::GA<__int32>::CB::CC"												);
	AssertType(pa,		L"GA<int>::CB::GC",					L"GA<int> :: CB :: GC",					L"<::GA::CB::GC::[TC]> ::GA<__int32>::CB::GC<::GA::CB::GC::[TC]>"		);
	AssertType(pa,		L"GA<int>::CB::GC<float>",			L"GA<int> :: CB :: GC<float>",			L"::GA<__int32>::CB::GC<float>"											);
	AssertType(pa,		L"GA<int>::GB<bool>::CC",			L"GA<int> :: GB<bool> :: CC",			L"::GA<__int32>::GB<bool>::CC"											);
	AssertType(pa,		L"GA<int>::GB<bool>::GC",			L"GA<int> :: GB<bool> :: GC",			L"<::GA::GB::GC::[TC]> ::GA<__int32>::GB<bool>::GC<::GA::GB::GC::[TC]>"	);
	AssertType(pa,		L"GA<int>::GB<bool>::GC<float>",	L"GA<int> :: GB<bool> :: GC<float>",	L"::GA<__int32>::GB<bool>::GC<float>"									);
}

TEST_CASE(TestParseGenericClass_NestedTypeAliasUsedOutside)
{
	auto input = LR"(
template<typename TA>
struct GA
{
	struct CB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};

		using _CC = CC;
		template<typename TC> using _GC = GC<TC>;
	};

	template<typename TB>
	struct GB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};

		using _CC = CC;
		template<typename TC> using _GC = GC<TC>;
	};

	using _CB = CB;
	template<typename TB> using _GB = GB<TB>;
};
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(pa,		L"GA",								L"GA",									L"<::GA::[TA]> ::GA<::GA::[TA]>"											);
	AssertType(pa,		L"GA<int>::_CB",					L"GA<int> :: _CB",						L"::GA<__int32>::CB"														);
	AssertType(pa,		L"GA<int>::_GB",					L"GA<int> :: _GB",						L"<::GA::_GB::[TB]> ::GA<__int32>::GB<::GA::_GB::[TB]>"						);
	AssertType(pa,		L"GA<int>::_GB<bool>",				L"GA<int> :: _GB<bool>",				L"::GA<__int32>::GB<bool>"													);
	AssertType(pa,		L"GA<int>::_CB::_CC",				L"GA<int> :: _CB :: _CC",				L"::GA<__int32>::CB::CC"													);
	AssertType(pa,		L"GA<int>::_CB::_GC",				L"GA<int> :: _CB :: _GC",				L"<::GA::CB::_GC::[TC]> ::GA<__int32>::CB::GC<::GA::CB::_GC::[TC]>"			);
	AssertType(pa,		L"GA<int>::_CB::_GC<float>",		L"GA<int> :: _CB :: _GC<float>",		L"::GA<__int32>::CB::GC<float>"												);
	AssertType(pa,		L"GA<int>::_GB<bool>::_CC",			L"GA<int> :: _GB<bool> :: _CC",			L"::GA<__int32>::GB<bool>::CC"												);
	AssertType(pa,		L"GA<int>::_GB<bool>::_GC",			L"GA<int> :: _GB<bool> :: _GC",			L"<::GA::GB::_GC::[TC]> ::GA<__int32>::GB<bool>::GC<::GA::GB::_GC::[TC]>"	);
	AssertType(pa,		L"GA<int>::_GB<bool>::_GC<float>",	L"GA<int> :: _GB<bool> :: _GC<float>",	L"::GA<__int32>::GB<bool>::GC<float>"										);
}

TEST_CASE(TestParseGenericClass_BaseThisType)
{
	auto input = LR"(
template<typename T>
struct A
{
	auto _A()					{ return this; }
	auto _A() const				{ return this; }
	auto _A() volatile			{ return this; }
	auto _A() const volatile	{ return this; }
};

struct B : A<int>
{
	auto _B()					{ return this; }
	auto _B() const				{ return this; }
	auto _B() volatile			{ return this; }
	auto _B() const volatile	{ return this; }
};

template<typename T>
struct C
{
	auto _C()					{ return this; }
	auto _C() const				{ return this; }
	auto _C() volatile			{ return this; }
	auto _C() const volatile	{ return this; }
};

template<typename T>
struct D : B, C<double>
{
	auto _D()					{ return this; }
	auto _D() const				{ return this; }
	auto _D() volatile			{ return this; }
	auto _D() const volatile	{ return this; }

	auto A_()					{ return _A(); }
	auto A_() const				{ return _A(); }
	auto A_() volatile			{ return _A(); }
	auto A_() const volatile	{ return _A(); }

	auto B_()					{ return _B(); }
	auto B_() const				{ return _B(); }
	auto B_() volatile			{ return _B(); }
	auto B_() const volatile	{ return _B(); }

	auto C_()					{ return _C(); }
	auto C_() const				{ return _C(); }
	auto C_() volatile			{ return _C(); }
	auto C_() const volatile	{ return _C(); }

	auto D_()					{ return _D(); }
	auto D_() const				{ return _D(); }
	auto D_() volatile			{ return _D(); }
	auto D_() const volatile	{ return _D(); }
};

D<char> d;
const D<char> cd;
volatile D<char> vd;
volatile const D<char> cvd;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(L"d._A()",		L"d._A()",			L"::A<__int32> *"					);
	AssertExpr(L"d._B()",		L"d._B()",			L"::B *"							);
	AssertExpr(L"d._C()",		L"d._C()",			L"::C<double> *"					);
	AssertExpr(L"d._D()",		L"d._D()",			L"::D<char> *"						);
	AssertExpr(L"d.A_()",		L"d.A_()",			L"::A<__int32> *"					);
	AssertExpr(L"d.B_()",		L"d.B_()",			L"::B *"							);
	AssertExpr(L"d.C_()",		L"d.C_()",			L"::C<double> *"					);
	AssertExpr(L"d.D_()",		L"d.D_()",			L"::D<char> *"						);

	AssertExpr(L"cd._A()",		L"cd._A()",			L"::A<__int32> const *"				);
	AssertExpr(L"cd._B()",		L"cd._B()",			L"::B const *"						);
	AssertExpr(L"cd._C()",		L"cd._C()",			L"::C<double> const *"				);
	AssertExpr(L"cd._D()",		L"cd._D()",			L"::D<char> const *"				);
	AssertExpr(L"cd.A_()",		L"cd.A_()",			L"::A<__int32> const *"				);
	AssertExpr(L"cd.B_()",		L"cd.B_()",			L"::B const *"						);
	AssertExpr(L"cd.C_()",		L"cd.C_()",			L"::C<double> const *"				);
	AssertExpr(L"cd.D_()",		L"cd.D_()",			L"::D<char> const *"				);

	AssertExpr(L"cd._A()",		L"cd._A()",			L"::A<__int32> volatile *"			);
	AssertExpr(L"cd._B()",		L"cd._B()",			L"::B volatile *"					);
	AssertExpr(L"cd._C()",		L"cd._C()",			L"::C<double> volatile *"			);
	AssertExpr(L"cd._D()",		L"cd._D()",			L"::D<char> volatile *"				);
	AssertExpr(L"cd.A_()",		L"cd.A_()",			L"::A<__int32> volatile *"			);
	AssertExpr(L"cd.B_()",		L"cd.B_()",			L"::B volatile *"					);
	AssertExpr(L"cd.C_()",		L"cd.C_()",			L"::C<double> volatile *"			);
	AssertExpr(L"cd.D_()",		L"cd.D_()",			L"::D<char> volatile *"				);

	AssertExpr(L"ccd._A()",		L"ccd._A()",		L"::A<__int32> const volatile *"	);
	AssertExpr(L"ccd._B()",		L"ccd._B()",		L"::B const volatile *"				);
	AssertExpr(L"ccd._C()",		L"ccd._C()",		L"::C<double> const volatile *"		);
	AssertExpr(L"ccd._D()",		L"ccd._D()",		L"::D<char> const volatile *"		);
	AssertExpr(L"ccd.A_()",		L"ccd.A_()",		L"::A<__int32> const volatile *"	);
	AssertExpr(L"ccd.B_()",		L"ccd.B_()",		L"::B const volatile *"				);
	AssertExpr(L"ccd.C_()",		L"ccd.C_()",		L"::C<double> const volatile *"		);
	AssertExpr(L"ccd.D_()",		L"ccd.D_()",		L"::D<char> const volatile *"		);
}

TEST_CASE(TestParseGenericClass_NestedThisType)
{
}

TEST_CASE(TestParseGenericClass_NestedStructUsedInside)
{
}

TEST_CASE(TestParseGenericClass_NestedTypeAliasUsedInside)
{
}

TEST_CASE(TestParseGenericClass_Overloading_BaseClass)
{
}

TEST_CASE(TestParseGenericClass_Overloading_SpecialMember)
{
}

TEST_CASE(TestParseGenericClass_Overloading_OperatorOverloading)
{
}