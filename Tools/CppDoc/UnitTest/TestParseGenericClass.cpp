#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Normal")
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
	});

	TEST_CATEGORY(L"Nested classes used from outside")
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
	});

	TEST_CATEGORY(L"Nested type aliases used from outside")
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
		using __CC = _CC;
		template<typename TC> using _GC = GC<TC>;
		template<typename TC> using __GC = _GC<TC>;
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
		using __CC = _CC;
		template<typename TC> using _GC = GC<TC>;
		template<typename TC> using __GC = _GC<TC>;
	};

	using _CB = CB;
	using __CB = _CB;
	template<typename TB> using _GB = GB<TB>;
	template<typename TB> using __GB = _GB<TB>;
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertType(pa,		L"GA",									L"GA",										L"<::GA::[TA]> ::GA<::GA::[TA]>"											);
		AssertType(pa,		L"GA<int>::__CB",						L"GA<int> :: __CB",							L"::GA<__int32>::CB"														);
		AssertType(pa,		L"GA<int>::__GB",						L"GA<int> :: __GB",							L"<::GA::__GB::[TB]> ::GA<__int32>::GB<::GA::__GB::[TB]>"					);
		AssertType(pa,		L"GA<int>::__GB<bool>",					L"GA<int> :: __GB<bool>",					L"::GA<__int32>::GB<bool>"													);
		AssertType(pa,		L"GA<int>::__CB::__CC",					L"GA<int> :: __CB :: __CC",					L"::GA<__int32>::CB::CC"													);
		AssertType(pa,		L"GA<int>::__CB::__GC",					L"GA<int> :: __CB :: __GC",					L"<::GA::CB::__GC::[TC]> ::GA<__int32>::CB::GC<::GA::CB::__GC::[TC]>"		);
		AssertType(pa,		L"GA<int>::__CB::__GC<float>",			L"GA<int> :: __CB :: __GC<float>",			L"::GA<__int32>::CB::GC<float>"												);
		AssertType(pa,		L"GA<int>::__GB<bool>::__CC",			L"GA<int> :: __GB<bool> :: __CC",			L"::GA<__int32>::GB<bool>::CC"												);
		AssertType(pa,		L"GA<int>::__GB<bool>::__GC",			L"GA<int> :: __GB<bool> :: __GC",			L"<::GA::GB::__GC::[TC]> ::GA<__int32>::GB<bool>::GC<::GA::GB::__GC::[TC]>"	);
		AssertType(pa,		L"GA<int>::__GB<bool>::__GC<float>",	L"GA<int> :: __GB<bool> :: __GC<float>",	L"::GA<__int32>::GB<bool>::GC<float>"										);
	});

	TEST_CATEGORY(L"This in base types")
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

		AssertExpr(pa,		L"d._A()",		L"d._A()",			L"::A<__int32> * $PR"					);
		AssertExpr(pa,		L"d._B()",		L"d._B()",			L"::B * $PR"							);
		AssertExpr(pa,		L"d._C()",		L"d._C()",			L"::C<double> * $PR"					);
		AssertExpr(pa,		L"d._D()",		L"d._D()",			L"::D<char> * $PR"						);
		AssertExpr(pa,		L"d.A_()",		L"d.A_()",			L"::A<__int32> * $PR"					);
		AssertExpr(pa,		L"d.B_()",		L"d.B_()",			L"::B * $PR"							);
		AssertExpr(pa,		L"d.C_()",		L"d.C_()",			L"::C<double> * $PR"					);
		AssertExpr(pa,		L"d.D_()",		L"d.D_()",			L"::D<char> * $PR"						);

		AssertExpr(pa,		L"cd._A()",		L"cd._A()",			L"::A<__int32> const * $PR"				);
		AssertExpr(pa,		L"cd._B()",		L"cd._B()",			L"::B const * $PR"						);
		AssertExpr(pa,		L"cd._C()",		L"cd._C()",			L"::C<double> const * $PR"				);
		AssertExpr(pa,		L"cd._D()",		L"cd._D()",			L"::D<char> const * $PR"				);
		AssertExpr(pa,		L"cd.A_()",		L"cd.A_()",			L"::A<__int32> const * $PR"				);
		AssertExpr(pa,		L"cd.B_()",		L"cd.B_()",			L"::B const * $PR"						);
		AssertExpr(pa,		L"cd.C_()",		L"cd.C_()",			L"::C<double> const * $PR"				);
		AssertExpr(pa,		L"cd.D_()",		L"cd.D_()",			L"::D<char> const * $PR"				);

		AssertExpr(pa,		L"vd._A()",		L"vd._A()",			L"::A<__int32> volatile * $PR"			);
		AssertExpr(pa,		L"vd._B()",		L"vd._B()",			L"::B volatile * $PR"					);
		AssertExpr(pa,		L"vd._C()",		L"vd._C()",			L"::C<double> volatile * $PR"			);
		AssertExpr(pa,		L"vd._D()",		L"vd._D()",			L"::D<char> volatile * $PR"				);
		AssertExpr(pa,		L"vd.A_()",		L"vd.A_()",			L"::A<__int32> volatile * $PR"			);
		AssertExpr(pa,		L"vd.B_()",		L"vd.B_()",			L"::B volatile * $PR"					);
		AssertExpr(pa,		L"vd.C_()",		L"vd.C_()",			L"::C<double> volatile * $PR"			);
		AssertExpr(pa,		L"vd.D_()",		L"vd.D_()",			L"::D<char> volatile * $PR"				);

		AssertExpr(pa,		L"cvd._A()",	L"cvd._A()",		L"::A<__int32> const volatile * $PR"	);
		AssertExpr(pa,		L"cvd._B()",	L"cvd._B()",		L"::B const volatile * $PR"				);
		AssertExpr(pa,		L"cvd._C()",	L"cvd._C()",		L"::C<double> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd._D()",	L"cvd._D()",		L"::D<char> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd.A_()",	L"cvd.A_()",		L"::A<__int32> const volatile * $PR"	);
		AssertExpr(pa,		L"cvd.B_()",	L"cvd.B_()",		L"::B const volatile * $PR"				);
		AssertExpr(pa,		L"cvd.C_()",	L"cvd.C_()",		L"::C<double> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd.D_()",	L"cvd.D_()",		L"::D<char> const volatile * $PR"		);
	});

	TEST_CATEGORY(L"This in nested classes")
	{
		auto input = LR"(
template<typename TX>
struct x
{
	template<typename T>
	struct A
	{
		auto _A()					{ return this; }
		auto _A() const				{ return this; }
		auto _A() volatile			{ return this; }
		auto _A() const volatile	{ return this; }
	};

	template<typename TY>
	struct y
	{
		struct B : A<int>
		{
			auto _B()					{ return this; }
			auto _B() const				{ return this; }
			auto _B() volatile			{ return this; }
			auto _B() const volatile	{ return this; }
		};

		struct C
		{
			auto _C()					{ return this; }
			auto _C() const				{ return this; }
			auto _C() volatile			{ return this; }
			auto _C() const volatile	{ return this; }
		};
	};

	template<typename T>
	struct D : y<float>::B, y<double>::C
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
};

x<void*>::D<char> d;
const x<void*>::D<char> cd;
volatile x<void*>::D<char> vd;
volatile const x<void*>::D<char> cvd;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,		L"d._A()",		L"d._A()",			L"::x<void *>::A<__int32> * $PR"							);
		AssertExpr(pa,		L"d._B()",		L"d._B()",			L"::x<void *>::y<float>::B * $PR"							);
		AssertExpr(pa,		L"d._C()",		L"d._C()",			L"::x<void *>::y<double>::C * $PR"							);
		AssertExpr(pa,		L"d._D()",		L"d._D()",			L"::x<void *>::D<char> * $PR"								);
		AssertExpr(pa,		L"d.A_()",		L"d.A_()",			L"::x<void *>::A<__int32> * $PR"							);
		AssertExpr(pa,		L"d.B_()",		L"d.B_()",			L"::x<void *>::y<float>::B * $PR"							);
		AssertExpr(pa,		L"d.C_()",		L"d.C_()",			L"::x<void *>::y<double>::C * $PR"							);
		AssertExpr(pa,		L"d.D_()",		L"d.D_()",			L"::x<void *>::D<char> * $PR"								);

		AssertExpr(pa,		L"cd._A()",		L"cd._A()",			L"::x<void *>::A<__int32> const * $PR"						);
		AssertExpr(pa,		L"cd._B()",		L"cd._B()",			L"::x<void *>::y<float>::B const * $PR"						);
		AssertExpr(pa,		L"cd._C()",		L"cd._C()",			L"::x<void *>::y<double>::C const * $PR"					);
		AssertExpr(pa,		L"cd._D()",		L"cd._D()",			L"::x<void *>::D<char> const * $PR"							);
		AssertExpr(pa,		L"cd.A_()",		L"cd.A_()",			L"::x<void *>::A<__int32> const * $PR"						);
		AssertExpr(pa,		L"cd.B_()",		L"cd.B_()",			L"::x<void *>::y<float>::B const * $PR"						);
		AssertExpr(pa,		L"cd.C_()",		L"cd.C_()",			L"::x<void *>::y<double>::C const * $PR"					);
		AssertExpr(pa,		L"cd.D_()",		L"cd.D_()",			L"::x<void *>::D<char> const * $PR"							);

		AssertExpr(pa,		L"vd._A()",		L"vd._A()",			L"::x<void *>::A<__int32> volatile * $PR"					);
		AssertExpr(pa,		L"vd._B()",		L"vd._B()",			L"::x<void *>::y<float>::B volatile * $PR"					);
		AssertExpr(pa,		L"vd._C()",		L"vd._C()",			L"::x<void *>::y<double>::C volatile * $PR"					);
		AssertExpr(pa,		L"vd._D()",		L"vd._D()",			L"::x<void *>::D<char> volatile * $PR"						);
		AssertExpr(pa,		L"vd.A_()",		L"vd.A_()",			L"::x<void *>::A<__int32> volatile * $PR"					);
		AssertExpr(pa,		L"vd.B_()",		L"vd.B_()",			L"::x<void *>::y<float>::B volatile * $PR"					);
		AssertExpr(pa,		L"vd.C_()",		L"vd.C_()",			L"::x<void *>::y<double>::C volatile * $PR"					);
		AssertExpr(pa,		L"vd.D_()",		L"vd.D_()",			L"::x<void *>::D<char> volatile * $PR"						);

		AssertExpr(pa,		L"cvd._A()",	L"cvd._A()",		L"::x<void *>::A<__int32> const volatile * $PR"				);
		AssertExpr(pa,		L"cvd._B()",	L"cvd._B()",		L"::x<void *>::y<float>::B const volatile * $PR"			);
		AssertExpr(pa,		L"cvd._C()",	L"cvd._C()",		L"::x<void *>::y<double>::C const volatile * $PR"			);
		AssertExpr(pa,		L"cvd._D()",	L"cvd._D()",		L"::x<void *>::D<char> const volatile * $PR"				);
		AssertExpr(pa,		L"cvd.A_()",	L"cvd.A_()",		L"::x<void *>::A<__int32> const volatile * $PR"				);
		AssertExpr(pa,		L"cvd.B_()",	L"cvd.B_()",		L"::x<void *>::y<float>::B const volatile * $PR"			);
		AssertExpr(pa,		L"cvd.C_()",	L"cvd.C_()",		L"::x<void *>::y<double>::C const volatile * $PR"			);
		AssertExpr(pa,		L"cvd.D_()",	L"cvd.D_()",		L"::x<void *>::D<char> const volatile * $PR"				);
	});

	TEST_CATEGORY(L"Declaration after evaluation")
	{
		auto input = LR"(
template<typename T>
struct X;

X<int> x;

template<typename U>
struct X
{
	U* y;
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,		L"x.y",			L"x.y",				L"__int32 * $L"	);
	});
}