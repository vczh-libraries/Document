#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Normal")
	{
		auto input = LR"(
template<typename T>
struct Ptr {};

template<typename U>
struct Ptr<U[]> {};
)";

		COMPILE_PROGRAM(program, pa, input);

		AssertType(pa,		L"Ptr",			L"Ptr",			L"<::Ptr::[T]> ::Ptr<::Ptr::[T]>");
		AssertType(pa,		L"Ptr<int[]>",	L"Ptr<int []>",	L"::Ptr<__int32 []>");
	});

	TEST_CATEGORY(L"Nested classes used from outside")
	{
		auto input = LR"(
template<typename TA>
struct GA
{
};

template<>
struct GA<int>
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

		template<>
		struct GC<float>
		{
		};
	};

	template<typename TB>
	struct GB
	{
	};

	template<>
	struct GB<bool>
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};

		template<>
		struct GC<float>
		{
		};
	};
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertType(pa,		L"GA",								L"GA",									L"<::GA::[TA]> ::GA<::GA::[TA]>"																);
		AssertType(pa,		L"GA<int>::CB",						L"GA<int> :: CB",						L"::GA@<int>::CB"																				);
		AssertType(pa,		L"GA<int>::GB",						L"GA<int> :: GB",						L"<::GA@<int>::GB::[TB]> ::GA@<int>::GB<::GA@<int>::GB::[TB]>"									);
		AssertType(pa,		L"GA<int>::GB<bool>",				L"GA<int> :: GB<bool>",					L"::GA@<int>::GB<bool>"																			);
		AssertType(pa,		L"GA<int>::CB::CC",					L"GA<int> :: CB :: CC",					L"::GA@<int>::CB::CC"																			);
		AssertType(pa,		L"GA<int>::CB::GC",					L"GA<int> :: CB :: GC",					L"<::GA@<int>::CB::GC::[TC]> ::GA@<int>::CB::GC<::GA@<int>::CB::GC::[TC]>"						);
		AssertType(pa,		L"GA<int>::CB::GC<float>",			L"GA<int> :: CB :: GC<float>",			L"::GA@<int>::CB::GC<float>"																	);
		AssertType(pa,		L"GA<int>::GB<bool>::CC",			L"GA<int> :: GB<bool> :: CC",			L"::GA@<int>::GB@<bool>::CC"																	);
		AssertType(pa,		L"GA<int>::GB<bool>::GC",			L"GA<int> :: GB<bool> :: GC",			L"<::GA@<int>::GB@<bool>::GC::[TC]> ::GA@<int>::GB@<bool>::GC<::GA@<int>::GB@<bool>::GC::[TC]>"	);
		AssertType(pa,		L"GA<int>::GB<bool>::GC<float>",	L"GA<int> :: GB<bool> :: GC<float>",	L"::GA@<int>::GB@<bool>::GC<float>"																);
	});

	TEST_CATEGORY(L"Nested type aliases used from outside")
	{
		auto input = LR"(
template<typename TA>
struct GA
{
};

template<>
struct GA<int>
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

		template<>
		struct GC<float>
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
	};

	template<>
	struct GB<bool>
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};

		template<>
		struct GC<float>
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

		AssertType(pa,		L"GA",									L"GA",										L"<::GA::[TA]> ::GA<::GA::[TA]>"																	);
		AssertType(pa,		L"GA<int>::__CB",						L"GA<int> :: __CB",							L"::GA@<int>::CB"																					);
		AssertType(pa,		L"GA<int>::__GB",						L"GA<int> :: __GB",							L"<::GA@<int>::__GB::[TB]> ::GA@<int>::GB<::GA@<int>::__GB::[TB]>"									);
		AssertType(pa,		L"GA<int>::__GB<bool>",					L"GA<int> :: __GB<bool>",					L"::GA@<int>::GB<bool>"																				);
		AssertType(pa,		L"GA<int>::__CB::__CC",					L"GA<int> :: __CB :: __CC",					L"::GA@<int>::CB::CC"																				);
		AssertType(pa,		L"GA<int>::__CB::__GC",					L"GA<int> :: __CB :: __GC",					L"<::GA@<int>::CB::__GC::[TC]> ::GA@<int>::CB::GC<::GA@<int>::CB::__GC::[TC]>"						);
		AssertType(pa,		L"GA<int>::__CB::__GC<float>",			L"GA<int> :: __CB :: __GC<float>",			L"::GA@<int>::CB::GC<float>"																		);
		AssertType(pa,		L"GA<int>::__GB<bool>::__CC",			L"GA<int> :: __GB<bool> :: __CC",			L"::GA@<int>::GB@<bool>::CC"																		);
		AssertType(pa,		L"GA<int>::__GB<bool>::__GC",			L"GA<int> :: __GB<bool> :: __GC",			L"<::GA@<int>::GB@<bool>::__GC::[TC]> ::GA@<int>::GB@<bool>::GC<::GA@<int>::GB@<bool>::__GC::[TC]>"	);
		AssertType(pa,		L"GA<int>::__GB<bool>::__GC<float>",	L"GA<int> :: __GB<bool> :: __GC<float>",	L"::GA@<int>::GB@<bool>::GC<float>"																	);
	});

	TEST_CATEGORY(L"This in base types")
	{
		auto input = LR"(
template<typename T>
struct A
{
};

template<>
struct A<int>
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
};

template<>
struct C<double>
{
	auto _C()					{ return this; }
	auto _C() const				{ return this; }
	auto _C() volatile			{ return this; }
	auto _C() const volatile	{ return this; }
};

template<typename T>
struct D
{
};

template<>
struct D<char> : B, C<double>
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

		AssertExpr(pa,		L"d._A()",		L"d._A()",			L"::A@<int> * $PR"						);
		AssertExpr(pa,		L"d._B()",		L"d._B()",			L"::B * $PR"							);
		AssertExpr(pa,		L"d._C()",		L"d._C()",			L"::C@<double> * $PR"					);
		AssertExpr(pa,		L"d._D()",		L"d._D()",			L"::D@<char> * $PR"						);
		AssertExpr(pa,		L"d.A_()",		L"d.A_()",			L"::A@<int> * $PR"						);
		AssertExpr(pa,		L"d.B_()",		L"d.B_()",			L"::B * $PR"							);
		AssertExpr(pa,		L"d.C_()",		L"d.C_()",			L"::C@<double> * $PR"					);
		AssertExpr(pa,		L"d.D_()",		L"d.D_()",			L"::D@<char> * $PR"						);

		AssertExpr(pa,		L"cd._A()",		L"cd._A()",			L"::A@<int> const * $PR"				);
		AssertExpr(pa,		L"cd._B()",		L"cd._B()",			L"::B const * $PR"						);
		AssertExpr(pa,		L"cd._C()",		L"cd._C()",			L"::C@<double> const * $PR"				);
		AssertExpr(pa,		L"cd._D()",		L"cd._D()",			L"::D@<char> const * $PR"				);
		AssertExpr(pa,		L"cd.A_()",		L"cd.A_()",			L"::A@<int> const * $PR"				);
		AssertExpr(pa,		L"cd.B_()",		L"cd.B_()",			L"::B const * $PR"						);
		AssertExpr(pa,		L"cd.C_()",		L"cd.C_()",			L"::C@<double> const * $PR"				);
		AssertExpr(pa,		L"cd.D_()",		L"cd.D_()",			L"::D@<char> const * $PR"				);

		AssertExpr(pa,		L"vd._A()",		L"vd._A()",			L"::A@<int> volatile * $PR"				);
		AssertExpr(pa,		L"vd._B()",		L"vd._B()",			L"::B volatile * $PR"					);
		AssertExpr(pa,		L"vd._C()",		L"vd._C()",			L"::C@<double> volatile * $PR"			);
		AssertExpr(pa,		L"vd._D()",		L"vd._D()",			L"::D@<char> volatile * $PR"			);
		AssertExpr(pa,		L"vd.A_()",		L"vd.A_()",			L"::A@<int> volatile * $PR"				);
		AssertExpr(pa,		L"vd.B_()",		L"vd.B_()",			L"::B volatile * $PR"					);
		AssertExpr(pa,		L"vd.C_()",		L"vd.C_()",			L"::C@<double> volatile * $PR"			);
		AssertExpr(pa,		L"vd.D_()",		L"vd.D_()",			L"::D@<char> volatile * $PR"			);

		AssertExpr(pa,		L"cvd._A()",	L"cvd._A()",		L"::A@<int> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd._B()",	L"cvd._B()",		L"::B const volatile * $PR"				);
		AssertExpr(pa,		L"cvd._C()",	L"cvd._C()",		L"::C@<double> const volatile * $PR"	);
		AssertExpr(pa,		L"cvd._D()",	L"cvd._D()",		L"::D@<char> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd.A_()",	L"cvd.A_()",		L"::A@<int> const volatile * $PR"		);
		AssertExpr(pa,		L"cvd.B_()",	L"cvd.B_()",		L"::B const volatile * $PR"				);
		AssertExpr(pa,		L"cvd.C_()",	L"cvd.C_()",		L"::C@<double> const volatile * $PR"	);
		AssertExpr(pa,		L"cvd.D_()",	L"cvd.D_()",		L"::D@<char> const volatile * $PR"		);
	});

	TEST_CATEGORY(L"This in nested classes")
	{
		auto input = LR"(
template<typename TX>
struct x
{
};

template<typename T>
struct x<T*>
{
	template<typename T>
	struct A
	{
	};

	template<>
	struct A<int>
	{
		auto _A()					{ return this; }
		auto _A() const				{ return this; }
		auto _A() volatile			{ return this; }
		auto _A() const volatile	{ return this; }
	};

	template<typename TY>
	struct y
	{
	};

	template<>
	struct y<float>
	{
		struct B : A<int>
		{
			auto _B()					{ return this; }
			auto _B() const				{ return this; }
			auto _B() volatile			{ return this; }
			auto _B() const volatile	{ return this; }
		};
	};

	template<>
	struct y<double>
	{
		struct C
		{
			auto _C()					{ return this; }
			auto _C() const				{ return this; }
			auto _C() volatile			{ return this; }
			auto _C() const volatile	{ return this; }
		};
	};

	template<typename T>
	struct D
	{
	};

	template<>
	struct D<char> : y<float>::B, y<double>::C
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

		AssertExpr(pa,		L"d._A()",		L"d._A()",			L"::x@<[T] *><void>::A@<int> * $PR"							);
		AssertExpr(pa,		L"d._B()",		L"d._B()",			L"::x@<[T] *><void>::y@<float>::B * $PR"					);
		AssertExpr(pa,		L"d._C()",		L"d._C()",			L"::x@<[T] *><void>::y@<double>::C * $PR"					);
		AssertExpr(pa,		L"d._D()",		L"d._D()",			L"::x@<[T] *><void>::D@<char> * $PR"						);
		AssertExpr(pa,		L"d.A_()",		L"d.A_()",			L"::x@<[T] *><void>::A@<int> * $PR"							);
		AssertExpr(pa,		L"d.B_()",		L"d.B_()",			L"::x@<[T] *><void>::y@<float>::B * $PR"					);
		AssertExpr(pa,		L"d.C_()",		L"d.C_()",			L"::x@<[T] *><void>::y@<double>::C * $PR"					);
		AssertExpr(pa,		L"d.D_()",		L"d.D_()",			L"::x@<[T] *><void>::D@<char> * $PR"						);

		AssertExpr(pa,		L"cd._A()",		L"cd._A()",			L"::x@<[T] *><void>::A@<int> const * $PR"					);
		AssertExpr(pa,		L"cd._B()",		L"cd._B()",			L"::x@<[T] *><void>::y@<float>::B const * $PR"				);
		AssertExpr(pa,		L"cd._C()",		L"cd._C()",			L"::x@<[T] *><void>::y@<double>::C const * $PR"				);
		AssertExpr(pa,		L"cd._D()",		L"cd._D()",			L"::x@<[T] *><void>::D@<char> const * $PR"					);
		AssertExpr(pa,		L"cd.A_()",		L"cd.A_()",			L"::x@<[T] *><void>::A@<int> const * $PR"					);
		AssertExpr(pa,		L"cd.B_()",		L"cd.B_()",			L"::x@<[T] *><void>::y@<float>::B const * $PR"				);
		AssertExpr(pa,		L"cd.C_()",		L"cd.C_()",			L"::x@<[T] *><void>::y@<double>::C const * $PR"				);
		AssertExpr(pa,		L"cd.D_()",		L"cd.D_()",			L"::x@<[T] *><void>::D@<char> const * $PR"					);

		AssertExpr(pa,		L"vd._A()",		L"vd._A()",			L"::x@<[T] *><void>::A@<int> volatile * $PR"				);
		AssertExpr(pa,		L"vd._B()",		L"vd._B()",			L"::x@<[T] *><void>::y@<float>::B volatile * $PR"			);
		AssertExpr(pa,		L"vd._C()",		L"vd._C()",			L"::x@<[T] *><void>::y@<double>::C volatile * $PR"			);
		AssertExpr(pa,		L"vd._D()",		L"vd._D()",			L"::x@<[T] *><void>::D@<char> volatile * $PR"				);
		AssertExpr(pa,		L"vd.A_()",		L"vd.A_()",			L"::x@<[T] *><void>::A@<int> volatile * $PR"				);
		AssertExpr(pa,		L"vd.B_()",		L"vd.B_()",			L"::x@<[T] *><void>::y@<float>::B volatile * $PR"			);
		AssertExpr(pa,		L"vd.C_()",		L"vd.C_()",			L"::x@<[T] *><void>::y@<double>::C volatile * $PR"			);
		AssertExpr(pa,		L"vd.D_()",		L"vd.D_()",			L"::x@<[T] *><void>::D@<char> volatile * $PR"				);

		AssertExpr(pa,		L"cvd._A()",	L"cvd._A()",		L"::x@<[T] *><void>::A@<int> const volatile * $PR"			);
		AssertExpr(pa,		L"cvd._B()",	L"cvd._B()",		L"::x@<[T] *><void>::y@<float>::B const volatile * $PR"		);
		AssertExpr(pa,		L"cvd._C()",	L"cvd._C()",		L"::x@<[T] *><void>::y@<double>::C const volatile * $PR"	);
		AssertExpr(pa,		L"cvd._D()",	L"cvd._D()",		L"::x@<[T] *><void>::D@<char> const volatile * $PR"			);
		AssertExpr(pa,		L"cvd.A_()",	L"cvd.A_()",		L"::x@<[T] *><void>::A@<int> const volatile * $PR"			);
		AssertExpr(pa,		L"cvd.B_()",	L"cvd.B_()",		L"::x@<[T] *><void>::y@<float>::B const volatile * $PR"		);
		AssertExpr(pa,		L"cvd.C_()",	L"cvd.C_()",		L"::x@<[T] *><void>::y@<double>::C const volatile * $PR"	);
		AssertExpr(pa,		L"cvd.D_()",	L"cvd.D_()",		L"::x@<[T] *><void>::D@<char> const volatile * $PR"			);
	});
}