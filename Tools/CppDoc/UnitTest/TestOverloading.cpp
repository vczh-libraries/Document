#include "Util.h"

#pragma warning (push)
#pragma warning (disable: 4101)
#pragma warning (disable: 5046)

template<typename T, typename U>
struct IntIfSameType
{
	using Type = void;
};

template<typename T>
struct IntIfSameType<T, T>
{
	using Type = int;
};

template<typename T, typename U>
void RunOverloading()
{
	typename IntIfSameType<T, U>::Type test = 0;
}

#define ASSERT_OVERLOADING(INPUT, OUTPUT, TYPE)\
	RunOverloading<TYPE, decltype(INPUT)>, \
	AssertExpr(L#INPUT, OUTPUT, L#TYPE " $PR", pa)\

TEST_CASE(TestParseExpr_Overloading_Ref)
{
	{
		TEST_DECL(
struct X {};
double F(const X&);
bool F(X&);
char F(X&&);
X x;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(static_cast<const X&>(x)),		L"F(static_cast<X const &>(x))",		double);
		ASSERT_OVERLOADING(F(static_cast<X&>(x)),			L"F(static_cast<X &>(x))",				bool);
		ASSERT_OVERLOADING(F(static_cast<X&&>(x)),			L"F(static_cast<X &&>(x))",				char);
		ASSERT_OVERLOADING(F(x),							L"F(x)",								bool);
	}
}

TEST_CASE(TestParseExpr_Overloading_Array)
{
	{
		TEST_DECL(
double F(const volatile int*);
float F(const int*);
bool F(volatile int*);
char F(int*);

extern const volatile int a[1];
extern const int b[1];
extern volatile int c[1];
extern int d[1];
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(a),							L"F(a)",								double);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								float);
		ASSERT_OVERLOADING(F(c),							L"F(c)",								bool);
		ASSERT_OVERLOADING(F(d),							L"F(d)",								char);
	}
	{
		TEST_DECL(
double F(const volatile int[]);
float F(const int[]);
bool F(volatile int[]);
char F(int[]);

extern const volatile int a[1];
extern const int b[1];
extern volatile int c[1];
extern int d[1];
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(a),							L"F(a)",								double);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								float);
		ASSERT_OVERLOADING(F(c),							L"F(c)",								bool);
		ASSERT_OVERLOADING(F(d),							L"F(d)",								char);
	}
}

TEST_CASE(TestParseExpr_Overloading_EnumItem)
{
	{
		TEST_DECL(
enum A { a };
enum B { b };
enum class C { c };
enum class D { d };

bool F(A);
char F(B);
wchar_t F(C);
float F(D);
double F(int);

char G(char);
bool G(int);
double G(double);
		);
		COMPILE_PROGRAM(program, pa, input);
		
		ASSERT_OVERLOADING(F(0),							L"F(0)",								double);
		ASSERT_OVERLOADING(F(a),							L"F(a)",								bool);
		ASSERT_OVERLOADING(F(b),							L"F(b)",								char);
		ASSERT_OVERLOADING(F(A::a),							L"F(A :: a)",							bool);
		ASSERT_OVERLOADING(F(B::b),							L"F(B :: b)",							char);
		ASSERT_OVERLOADING(F(C::c),							L"F(C :: c)",							wchar_t);
		ASSERT_OVERLOADING(F(D::d),							L"F(D :: d)",							float);

		ASSERT_OVERLOADING(G('a'),							L"G('a')",								char);
		ASSERT_OVERLOADING(G(0),							L"G(0)",								bool);
		ASSERT_OVERLOADING(G(a),							L"G(a)",								bool);
		ASSERT_OVERLOADING(G(b),							L"G(b)",								bool);
		ASSERT_OVERLOADING(G(0.0),							L"G(0.0)",								double);
	}
}

TEST_CASE(TestParseExpr_Overloading_DefaultParameter)
{
	{
		TEST_DECL(
bool F(void*);
char F(int, int);
wchar_t F(int, double = 0);
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(0),							L"F(0)",								wchar_t);
		ASSERT_OVERLOADING(F(nullptr),						L"F(nullptr)",							bool);
		ASSERT_OVERLOADING(F(0,0),							L"F(0, 0)",								char);
		ASSERT_OVERLOADING(F(0,0.0),						L"F(0, 0.0)",							wchar_t);
		ASSERT_OVERLOADING(F(0,0.0f),						L"F(0, 0.0f)",							wchar_t);
	}
}

TEST_CASE(TestParseExpr_Overloading_VariantArguments)
{
	{
		TEST_DECL(
char F(int, ...);
wchar_t F(double, ...);
float F(int, double);
double F(double, double);
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(0, 0),							L"F(0, 0)",								float);
		ASSERT_OVERLOADING(F(0, 0.0),						L"F(0, 0.0)",							float);
		ASSERT_OVERLOADING(F(0, 0.0f),						L"F(0, 0.0f)",							float);

		ASSERT_OVERLOADING(F(0.0, 0),						L"F(0.0, 0)",							double);
		ASSERT_OVERLOADING(F(0.0, 0.0),						L"F(0.0, 0.0)",							double);
		ASSERT_OVERLOADING(F(0.0, 0.0f),					L"F(0.0, 0.0f)",						double);
		
		ASSERT_OVERLOADING(F(0),							L"F(0)",								char);
		ASSERT_OVERLOADING(F(0.0),							L"F(0.0)",								wchar_t);
		ASSERT_OVERLOADING(F(0, nullptr),					L"F(0, nullptr)",						char);
		ASSERT_OVERLOADING(F(0.0, nullptr),					L"F(0.0, nullptr)",						wchar_t);
	}
}

TEST_CASE(TestParseExpr_Overloading_Inheritance)
{
	{
		TEST_DECL(
struct X {};
struct Y {};
struct Z :X {};

double F(X&);
bool F(Y&);
double G(X&);
bool G(Y&);
char G(const Z&);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
	}
}

TEST_CASE(TestParseExpr_Overloading_TypeConversion)
{
	{
		TEST_DECL(
struct X {};
struct Y {};
struct Z { operator X() { throw 0; } };

double F(X);
bool F(Y);
double G(X);
bool G(Y);
char G(Z);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
	}

	{
		TEST_DECL(
struct Z {};
struct X { X(const Z&) {} };
struct Y {};

double F(X);
bool F(Y);
double G(X);
bool G(Y);
char G(Z);
Z z;
		);
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(z),							L"F(z)",								double);
		ASSERT_OVERLOADING(G(z),							L"G(z)",								char);
	}
}

TEST_CASE(TestParseExpr_Overloading_Universal_Initialization)
{
}

TEST_CASE(TestParseExpr_Overloading_InitializationList)
{
}

TEST_CASE(TestParseExpr_Overloading_Lambda)
{
}

TEST_CASE(TestParseExpr_Overloading_CallOP_Ctor_UI_DPVA)
{
}

TEST_DECL_(

namespace a
{
	struct X
	{
		void* operator++	(int);
		void* operator--	(int);
		void* operator++	();
		void* operator--	();
		void* operator~		();
		void* operator!		();
		void* operator-		();
		void* operator+		();
		void* operator&		();
		void* operator*		();
		void* operator*		(int);
		void* operator/		(int);
		void* operator%		(int);
		void* operator+		(int);
		void* operator-		(int);
		void* operator<<	(int);
		void* operator>>	(int);
		void* operator==	(int);
		void* operator!=	(int);
		void* operator<		(int);
		void* operator<=	(int);
		void* operator>		(int);
		void* operator>=	(int);
		void* operator&		(int);
		void* operator|		(int);
		void* operator^		(int);
		void* operator&&	(int);
		void* operator||	(int);
		void* operator=		(int);
		void* operator*=	(int);
		void* operator/=	(int);
		void* operator%=	(int);
		void* operator+=	(int);
		void* operator-=	(int);
		void* operator<<=	(int);
		void* operator>>=	(int);
		void* operator&=	(int);
		void* operator|=	(int);
		void* operator^=	(int);

		bool* operator++	(int)const;
		bool* operator--	(int)const;
		bool* operator++	()const;
		bool* operator--	()const;
		bool* operator~		()const;
		bool* operator!		()const;
		bool* operator-		()const;
		bool* operator+		()const;
		bool* operator&		()const;
		bool* operator*		()const;
		bool* operator*		(int)const;
		bool* operator/		(int)const;
		bool* operator%		(int)const;
		bool* operator+		(int)const;
		bool* operator-		(int)const;
		bool* operator<<	(int)const;
		bool* operator>>	(int)const;
		bool* operator==	(int)const;
		bool* operator!=	(int)const;
		bool* operator<		(int)const;
		bool* operator<=	(int)const;
		bool* operator>		(int)const;
		bool* operator>=	(int)const;
		bool* operator&		(int)const;
		bool* operator|		(int)const;
		bool* operator^		(int)const;
		bool* operator&&	(int)const;
		bool* operator||	(int)const;
		bool* operator=		(int)const;
		bool* operator*=	(int)const;
		bool* operator/=	(int)const;
		bool* operator%=	(int)const;
		bool* operator+=	(int)const;
		bool* operator-=	(int)const;
		bool* operator<<=	(int)const;
		bool* operator>>=	(int)const;
		bool* operator&=	(int)const;
		bool* operator|=	(int)const;
		bool* operator^=	(int)const;
	};
}

, op_input_1);

TEST_DECL_(

namespace a
{
	struct Y
	{
	};
	void* operator++		(Y&, int);
	void* operator--		(Y&, int);
	void* operator++		(Y&);
	void* operator--		(Y&);
	void* operator~			(Y&);
	void* operator!			(Y&);
	void* operator-			(Y&);
	void* operator+			(Y&);
	void* operator&			(Y&);
	void* operator*			(Y&);
	void* operator*			(Y&, int);
	void* operator/			(Y&, int);
	void* operator%			(Y&, int);
	void* operator+			(Y&, int);
	void* operator-			(Y&, int);
	void* operator<<		(Y&, int);
	void* operator>>		(Y&, int);
	void* operator==		(Y&, int);
	void* operator!=		(Y&, int);
	void* operator<			(Y&, int);
	void* operator<=		(Y&, int);
	void* operator>			(Y&, int);
	void* operator>=		(Y&, int);
	void* operator&			(Y&, int);
	void* operator|			(Y&, int);
	void* operator^			(Y&, int);
	void* operator&&		(Y&, int);
	void* operator||		(Y&, int);
	void* operator*=		(Y&, int);
	void* operator/=		(Y&, int);
	void* operator%=		(Y&, int);
	void* operator+=		(Y&, int);
	void* operator-=		(Y&, int);
	void* operator<<=		(Y&, int);
	void* operator>>=		(Y&, int);
	void* operator&=		(Y&, int);
	void* operator|=		(Y&, int);
	void* operator^=		(Y&, int);
	void* operator*			(int, Y&);
	void* operator/			(int, Y&);
	void* operator%			(int, Y&);
	void* operator+			(int, Y&);
	void* operator-			(int, Y&);
	void* operator<<		(int, Y&);
	void* operator>>		(int, Y&);
	void* operator==		(int, Y&);
	void* operator!=		(int, Y&);
	void* operator<			(int, Y&);
	void* operator<=		(int, Y&);
	void* operator>			(int, Y&);
	void* operator>=		(int, Y&);
	void* operator&			(int, Y&);
	void* operator|			(int, Y&);
	void* operator^			(int, Y&);
	void* operator&&		(int, Y&);
	void* operator||		(int, Y&);
	void* operator*=		(int, Y&);
	void* operator/=		(int, Y&);
	void* operator%=		(int, Y&);
	void* operator+=		(int, Y&);
	void* operator-=		(int, Y&);
	void* operator<<=		(int, Y&);
	void* operator>>=		(int, Y&);
	void* operator&=		(int, Y&);
	void* operator|=		(int, Y&);
	void* operator^=		(int, Y&);

	bool* operator++		(const Y&, int);
	bool* operator--		(const Y&, int);
	bool* operator++		(const Y&);
	bool* operator--		(const Y&);
	bool* operator~			(const Y&);
	bool* operator!			(const Y&);
	bool* operator-			(const Y&);
	bool* operator+			(const Y&);
	bool* operator&			(const Y&);
	bool* operator*			(const Y&);
	bool* operator*			(const Y&, int);
	bool* operator/			(const Y&, int);
	bool* operator%			(const Y&, int);
	bool* operator+			(const Y&, int);
	bool* operator-			(const Y&, int);
	bool* operator<<		(const Y&, int);
	bool* operator>>		(const Y&, int);
	bool* operator==		(const Y&, int);
	bool* operator!=		(const Y&, int);
	bool* operator<			(const Y&, int);
	bool* operator<=		(const Y&, int);
	bool* operator>			(const Y&, int);
	bool* operator>=		(const Y&, int);
	bool* operator&			(const Y&, int);
	bool* operator|			(const Y&, int);
	bool* operator^			(const Y&, int);
	bool* operator&&		(const Y&, int);
	bool* operator||		(const Y&, int);
	bool* operator*=		(const Y&, int);
	bool* operator/=		(const Y&, int);
	bool* operator%=		(const Y&, int);
	bool* operator+=		(const Y&, int);
	bool* operator-=		(const Y&, int);
	bool* operator<<=		(const Y&, int);
	bool* operator>>=		(const Y&, int);
	bool* operator&=		(const Y&, int);
	bool* operator|=		(const Y&, int);
	bool* operator^=		(const Y&, int);
	bool* operator*			(int, const Y&);
	bool* operator/			(int, const Y&);
	bool* operator%			(int, const Y&);
	bool* operator+			(int, const Y&);
	bool* operator-			(int, const Y&);
	bool* operator<<		(int, const Y&);
	bool* operator>>		(int, const Y&);
	bool* operator==		(int, const Y&);
	bool* operator!=		(int, const Y&);
	bool* operator<			(int, const Y&);
	bool* operator<=		(int, const Y&);
	bool* operator>			(int, const Y&);
	bool* operator>=		(int, const Y&);
	bool* operator&			(int, const Y&);
	bool* operator|			(int, const Y&);
	bool* operator^			(int, const Y&);
	bool* operator&&		(int, const Y&);
	bool* operator||		(int, const Y&);
	bool* operator*=		(int, const Y&);
	bool* operator/=		(int, const Y&);
	bool* operator%=		(int, const Y&);
	bool* operator+=		(int, const Y&);
	bool* operator-=		(int, const Y&);
	bool* operator<<=		(int, const Y&);
	bool* operator>>=		(int, const Y&);
	bool* operator&=		(int, const Y&);
	bool* operator|=		(int, const Y&);
	bool* operator^=		(int, const Y&);
}

, op_input_2);

TEST_DECL_(

namespace a
{
	struct Z
	{
	};
}
void* operator++			(a::Z&, int);
void* operator--			(a::Z&, int);
void* operator++			(a::Z&);
void* operator--			(a::Z&);
void* operator~				(a::Z&);
void* operator!				(a::Z&);
void* operator-				(a::Z&);
void* operator+				(a::Z&);
void* operator&				(a::Z&);
void* operator*				(a::Z&);
void* operator*				(a::Z&, int);
void* operator/				(a::Z&, int);
void* operator%				(a::Z&, int);
void* operator+				(a::Z&, int);
void* operator-				(a::Z&, int);
void* operator<<			(a::Z&, int);
void* operator>>			(a::Z&, int);
void* operator==			(a::Z&, int);
void* operator!=			(a::Z&, int);
void* operator<				(a::Z&, int);
void* operator<=			(a::Z&, int);
void* operator>				(a::Z&, int);
void* operator>=			(a::Z&, int);
void* operator&				(a::Z&, int);
void* operator|				(a::Z&, int);
void* operator^				(a::Z&, int);
void* operator&&			(a::Z&, int);
void* operator||			(a::Z&, int);
void* operator*=			(a::Z&, int);
void* operator/=			(a::Z&, int);
void* operator%=			(a::Z&, int);
void* operator+=			(a::Z&, int);
void* operator-=			(a::Z&, int);
void* operator<<=			(a::Z&, int);
void* operator>>=			(a::Z&, int);
void* operator&=			(a::Z&, int);
void* operator|=			(a::Z&, int);
void* operator^=			(a::Z&, int);
void* operator*				(int, a::Z&);
void* operator/				(int, a::Z&);
void* operator%				(int, a::Z&);
void* operator+				(int, a::Z&);
void* operator-				(int, a::Z&);
void* operator<<			(int, a::Z&);
void* operator>>			(int, a::Z&);
void* operator==			(int, a::Z&);
void* operator!=			(int, a::Z&);
void* operator<				(int, a::Z&);
void* operator<=			(int, a::Z&);
void* operator>				(int, a::Z&);
void* operator>=			(int, a::Z&);
void* operator&				(int, a::Z&);
void* operator|				(int, a::Z&);
void* operator^				(int, a::Z&);
void* operator&&			(int, a::Z&);
void* operator||			(int, a::Z&);
void* operator*=			(int, a::Z&);
void* operator/=			(int, a::Z&);
void* operator%=			(int, a::Z&);
void* operator+=			(int, a::Z&);
void* operator-=			(int, a::Z&);
void* operator<<=			(int, a::Z&);
void* operator>>=			(int, a::Z&);
void* operator&=			(int, a::Z&);
void* operator|=			(int, a::Z&);
void* operator^=			(int, a::Z&);

bool* operator++			(const a::Z&, int);
bool* operator--			(const a::Z&, int);
bool* operator++			(const a::Z&);
bool* operator--			(const a::Z&);
bool* operator~				(const a::Z&);
bool* operator!				(const a::Z&);
bool* operator-				(const a::Z&);
bool* operator+				(const a::Z&);
bool* operator&				(const a::Z&);
bool* operator*				(const a::Z&);
bool* operator*				(const a::Z&, int);
bool* operator/				(const a::Z&, int);
bool* operator%				(const a::Z&, int);
bool* operator+				(const a::Z&, int);
bool* operator-				(const a::Z&, int);
bool* operator<<			(const a::Z&, int);
bool* operator>>			(const a::Z&, int);
bool* operator==			(const a::Z&, int);
bool* operator!=			(const a::Z&, int);
bool* operator<				(const a::Z&, int);
bool* operator<=			(const a::Z&, int);
bool* operator>				(const a::Z&, int);
bool* operator>=			(const a::Z&, int);
bool* operator&				(const a::Z&, int);
bool* operator|				(const a::Z&, int);
bool* operator^				(const a::Z&, int);
bool* operator&&			(const a::Z&, int);
bool* operator||			(const a::Z&, int);
bool* operator*=			(const a::Z&, int);
bool* operator/=			(const a::Z&, int);
bool* operator%=			(const a::Z&, int);
bool* operator+=			(const a::Z&, int);
bool* operator-=			(const a::Z&, int);
bool* operator<<=			(const a::Z&, int);
bool* operator>>=			(const a::Z&, int);
bool* operator&=			(const a::Z&, int);
bool* operator|=			(const a::Z&, int);
bool* operator^=			(const a::Z&, int);
bool* operator*				(int, const a::Z&);
bool* operator/				(int, const a::Z&);
bool* operator%				(int, const a::Z&);
bool* operator+				(int, const a::Z&);
bool* operator-				(int, const a::Z&);
bool* operator<<			(int, const a::Z&);
bool* operator>>			(int, const a::Z&);
bool* operator==			(int, const a::Z&);
bool* operator!=			(int, const a::Z&);
bool* operator<				(int, const a::Z&);
bool* operator<=			(int, const a::Z&);
bool* operator>				(int, const a::Z&);
bool* operator>=			(int, const a::Z&);
bool* operator&				(int, const a::Z&);
bool* operator|				(int, const a::Z&);
bool* operator^				(int, const a::Z&);
bool* operator&&			(int, const a::Z&);
bool* operator||			(int, const a::Z&);
bool* operator*=			(int, const a::Z&);
bool* operator/=			(int, const a::Z&);
bool* operator%=			(int, const a::Z&);
bool* operator+=			(int, const a::Z&);
bool* operator-=			(int, const a::Z&);
bool* operator<<=			(int, const a::Z&);
bool* operator>>=			(int, const a::Z&);
bool* operator&=			(int, const a::Z&);
bool* operator|=			(int, const a::Z&);
bool* operator^=			(int, const a::Z&);

, op_input_3);

TEST_DECL_(

extern a::X x;
extern a::Y y;
extern a::Z z;

extern const a::X cx;
extern const a::Y cy;
extern const a::Z cz;

, op_input_4);

TEST_CASE(TestParseExpr_Overloading_PostfixUnary)
{
	WString op_input = WString(op_input_1) + op_input_2 + op_input_3 + op_input_4;
	COMPILE_PROGRAM(program, pa, op_input.Buffer());

	ASSERT_OVERLOADING(x++,									L"(x ++)",								void*);
	ASSERT_OVERLOADING(x--,									L"(x --)",								void*);
	ASSERT_OVERLOADING(y++,									L"(y ++)",								void*);
	ASSERT_OVERLOADING(y--,									L"(y --)",								void*);
	ASSERT_OVERLOADING(z++,									L"(z ++)",								void*);
	ASSERT_OVERLOADING(z--,									L"(z --)",								void*);

	ASSERT_OVERLOADING(cx++,								L"(cx ++)",								bool*);
	ASSERT_OVERLOADING(cx--,								L"(cx --)",								bool*);
	ASSERT_OVERLOADING(cy++,								L"(cy ++)",								bool*);
	ASSERT_OVERLOADING(cy--,								L"(cy --)",								bool*);
	ASSERT_OVERLOADING(cz++,								L"(cz ++)",								bool*);
	ASSERT_OVERLOADING(cz--,								L"(cz --)",								bool*);
}

TEST_CASE(TestParseExpr_Overloading_PrefixUnary)
{
	WString op_input = WString(op_input_1) + op_input_2 + op_input_3 + op_input_4;
	COMPILE_PROGRAM(program, pa, op_input.Buffer());

	ASSERT_OVERLOADING(++x,									L"(++ x)",								void*);
	ASSERT_OVERLOADING(--x,									L"(-- x)",								void*);
	ASSERT_OVERLOADING(~x,									L"(~ x)",								void*);
	ASSERT_OVERLOADING(!x,									L"(! x)",								void*);
	ASSERT_OVERLOADING(-x,									L"(- x)",								void*);
	ASSERT_OVERLOADING(+x,									L"(+ x)",								void*);
	ASSERT_OVERLOADING(&x,									L"(& x)",								void*);
	ASSERT_OVERLOADING(*x,									L"(* x)",								void*);

	ASSERT_OVERLOADING(++y,									L"(++ y)",								void*);
	ASSERT_OVERLOADING(--y,									L"(-- y)",								void*);
	ASSERT_OVERLOADING(~y,									L"(~ y)",								void*);
	ASSERT_OVERLOADING(!y,									L"(! y)",								void*);
	ASSERT_OVERLOADING(-y,									L"(- y)",								void*);
	ASSERT_OVERLOADING(+y,									L"(+ y)",								void*);
	ASSERT_OVERLOADING(&y,									L"(& y)",								void*);
	ASSERT_OVERLOADING(*y,									L"(* y)",								void*);

	ASSERT_OVERLOADING(++z,									L"(++ z)",								void*);
	ASSERT_OVERLOADING(--z,									L"(-- z)",								void*);
	ASSERT_OVERLOADING(~z,									L"(~ z)",								void*);
	ASSERT_OVERLOADING(!z,									L"(! z)",								void*);
	ASSERT_OVERLOADING(-z,									L"(- z)",								void*);
	ASSERT_OVERLOADING(+z,									L"(+ z)",								void*);
	ASSERT_OVERLOADING(&z,									L"(& z)",								void*);
	ASSERT_OVERLOADING(*z,									L"(* z)",								void*);

	ASSERT_OVERLOADING(++cx,								L"(++ cx)",								bool*);
	ASSERT_OVERLOADING(--cx,								L"(-- cx)",								bool*);
	ASSERT_OVERLOADING(~cx,									L"(~ cx)",								bool*);
	ASSERT_OVERLOADING(!cx,									L"(! cx)",								bool*);
	ASSERT_OVERLOADING(-cx,									L"(- cx)",								bool*);
	ASSERT_OVERLOADING(+cx,									L"(+ cx)",								bool*);
	ASSERT_OVERLOADING(&cx,									L"(& cx)",								bool*);
	ASSERT_OVERLOADING(*cx,									L"(* cx)",								bool*);

	ASSERT_OVERLOADING(++cy,								L"(++ cy)",								bool*);
	ASSERT_OVERLOADING(--cy,								L"(-- cy)",								bool*);
	ASSERT_OVERLOADING(~cy,									L"(~ cy)",								bool*);
	ASSERT_OVERLOADING(!cy,									L"(! cy)",								bool*);
	ASSERT_OVERLOADING(-cy,									L"(- cy)",								bool*);
	ASSERT_OVERLOADING(+cy,									L"(+ cy)",								bool*);
	ASSERT_OVERLOADING(&cy,									L"(& cy)",								bool*);
	ASSERT_OVERLOADING(*cy,									L"(* cy)",								bool*);

	ASSERT_OVERLOADING(++cz,								L"(++ cz)",								bool*);
	ASSERT_OVERLOADING(--cz,								L"(-- cz)",								bool*);
	ASSERT_OVERLOADING(~cz,									L"(~ cz)",								bool*);
	ASSERT_OVERLOADING(!cz,									L"(! cz)",								bool*);
	ASSERT_OVERLOADING(-cz,									L"(- cz)",								bool*);
	ASSERT_OVERLOADING(+cz,									L"(+ cz)",								bool*);
	ASSERT_OVERLOADING(&cz,									L"(& cz)",								bool*);
	ASSERT_OVERLOADING(*cz,									L"(* cz)",								bool*);
}

TEST_CASE(TestParseExpr_Overloading_Binary)
{
	WString op_input = WString(op_input_1) + op_input_2 + op_input_3 + op_input_4;
	COMPILE_PROGRAM(program, pa, op_input.Buffer());

	ASSERT_OVERLOADING(x* 0,								L"(x * 0)",								void*);
	ASSERT_OVERLOADING(x/ 0,								L"(x / 0)",								void*);
	ASSERT_OVERLOADING(x% 0,								L"(x % 0)",								void*);
	ASSERT_OVERLOADING(x+ 0,								L"(x + 0)",								void*);
	ASSERT_OVERLOADING(x- 0,								L"(x - 0)",								void*);
	ASSERT_OVERLOADING(x<<0,								L"(x << 0)",							void*);
	ASSERT_OVERLOADING(x>>0,								L"(x >> 0)",							void*);
	ASSERT_OVERLOADING(x==0,								L"(x == 0)",							void*);
	ASSERT_OVERLOADING(x!=0,								L"(x != 0)",							void*);
	ASSERT_OVERLOADING(x< 0,								L"(x < 0)",								void*);
	ASSERT_OVERLOADING(x<=0,								L"(x <= 0)",							void*);
	ASSERT_OVERLOADING(x> 0,								L"(x > 0)",								void*);
	ASSERT_OVERLOADING(x>=0,								L"(x >= 0)",							void*);
	ASSERT_OVERLOADING(x& 0,								L"(x & 0)",								void*);
	ASSERT_OVERLOADING(x| 0,								L"(x | 0)",								void*);
	ASSERT_OVERLOADING(x^ 0,								L"(x ^ 0)",								void*);
	ASSERT_OVERLOADING(x&&0,								L"(x && 0)",							void*);
	ASSERT_OVERLOADING(x||0,								L"(x || 0)",							void*);
	ASSERT_OVERLOADING(x= 0,								L"(x = 0)",								void*);
	ASSERT_OVERLOADING(x*=0,								L"(x *= 0)",							void*);
	ASSERT_OVERLOADING(x/=0,								L"(x /= 0)",							void*);
	ASSERT_OVERLOADING(x%=0,								L"(x %= 0)",							void*);
	ASSERT_OVERLOADING(x+=0,								L"(x += 0)",							void*);
	ASSERT_OVERLOADING(x-=0,								L"(x -= 0)",							void*);
	ASSERT_OVERLOADING(x<<0,								L"(x << 0)",							void*);
	ASSERT_OVERLOADING(x>>0,								L"(x >> 0)",							void*);
	ASSERT_OVERLOADING(x&=0,								L"(x &= 0)",							void*);
	ASSERT_OVERLOADING(x|=0,								L"(x |= 0)",							void*);
	ASSERT_OVERLOADING(x^=0,								L"(x ^= 0)",							void*);

	ASSERT_OVERLOADING(y* 0,								L"(y * 0)",								void*);
	ASSERT_OVERLOADING(y/ 0,								L"(y / 0)",								void*);
	ASSERT_OVERLOADING(y% 0,								L"(y % 0)",								void*);
	ASSERT_OVERLOADING(y+ 0,								L"(y + 0)",								void*);
	ASSERT_OVERLOADING(y- 0,								L"(y - 0)",								void*);
	ASSERT_OVERLOADING(y<<0,								L"(y << 0)",							void*);
	ASSERT_OVERLOADING(y>>0,								L"(y >> 0)",							void*);
	ASSERT_OVERLOADING(y==0,								L"(y == 0)",							void*);
	ASSERT_OVERLOADING(y!=0,								L"(y != 0)",							void*);
	ASSERT_OVERLOADING(y< 0,								L"(y < 0)",								void*);
	ASSERT_OVERLOADING(y<=0,								L"(y <= 0)",							void*);
	ASSERT_OVERLOADING(y> 0,								L"(y > 0)",								void*);
	ASSERT_OVERLOADING(y>=0,								L"(y >= 0)",							void*);
	ASSERT_OVERLOADING(y& 0,								L"(y & 0)",								void*);
	ASSERT_OVERLOADING(y| 0,								L"(y | 0)",								void*);
	ASSERT_OVERLOADING(y^ 0,								L"(y ^ 0)",								void*);
	ASSERT_OVERLOADING(y&&0,								L"(y && 0)",							void*);
	ASSERT_OVERLOADING(y||0,								L"(y || 0)",							void*);
	ASSERT_OVERLOADING(y*=0,								L"(y *= 0)",							void*);
	ASSERT_OVERLOADING(y/=0,								L"(y /= 0)",							void*);
	ASSERT_OVERLOADING(y%=0,								L"(y %= 0)",							void*);
	ASSERT_OVERLOADING(y+=0,								L"(y += 0)",							void*);
	ASSERT_OVERLOADING(y-=0,								L"(y -= 0)",							void*);
	ASSERT_OVERLOADING(y<<0,								L"(y << 0)",							void*);
	ASSERT_OVERLOADING(y>>0,								L"(y >> 0)",							void*);
	ASSERT_OVERLOADING(y&=0,								L"(y &= 0)",							void*);
	ASSERT_OVERLOADING(y|=0,								L"(y |= 0)",							void*);
	ASSERT_OVERLOADING(y^=0,								L"(y ^= 0)",							void*);

	ASSERT_OVERLOADING(0* y,								L"(0 * y)",								void*);
	ASSERT_OVERLOADING(0/ y,								L"(0 / y)",								void*);
	ASSERT_OVERLOADING(0% y,								L"(0 % y)",								void*);
	ASSERT_OVERLOADING(0+ y,								L"(0 + y)",								void*);
	ASSERT_OVERLOADING(0- y,								L"(0 - y)",								void*);
	ASSERT_OVERLOADING(0<<y,								L"(0 << y)",							void*);
	ASSERT_OVERLOADING(0>>y,								L"(0 >> y)",							void*);
	ASSERT_OVERLOADING(0==y,								L"(0 == y)",							void*);
	ASSERT_OVERLOADING(0!=y,								L"(0 != y)",							void*);
	ASSERT_OVERLOADING(0< y,								L"(0 < y)",								void*);
	ASSERT_OVERLOADING(0<=y,								L"(0 <= y)",							void*);
	ASSERT_OVERLOADING(0> y,								L"(0 > y)",								void*);
	ASSERT_OVERLOADING(0>=y,								L"(0 >= y)",							void*);
	ASSERT_OVERLOADING(0& y,								L"(0 & y)",								void*);
	ASSERT_OVERLOADING(0| y,								L"(0 | y)",								void*);
	ASSERT_OVERLOADING(0^ y,								L"(0 ^ y)",								void*);
	ASSERT_OVERLOADING(0&&y,								L"(0 && y)",							void*);
	ASSERT_OVERLOADING(0||y,								L"(0 || y)",							void*);
	ASSERT_OVERLOADING(0*=y,								L"(0 *= y)",							void*);
	ASSERT_OVERLOADING(0/=y,								L"(0 /= y)",							void*);
	ASSERT_OVERLOADING(0%=y,								L"(0 %= y)",							void*);
	ASSERT_OVERLOADING(0+=y,								L"(0 += y)",							void*);
	ASSERT_OVERLOADING(0-=y,								L"(0 -= y)",							void*);
	ASSERT_OVERLOADING(0<<y,								L"(0 << y)",							void*);
	ASSERT_OVERLOADING(0>>y,								L"(0 >> y)",							void*);
	ASSERT_OVERLOADING(0&=y,								L"(0 &= y)",							void*);
	ASSERT_OVERLOADING(0|=y,								L"(0 |= y)",							void*);
	ASSERT_OVERLOADING(0^=y,								L"(0 ^= y)",							void*);

	ASSERT_OVERLOADING(z* 0,								L"(z * 0)",								void*);
	ASSERT_OVERLOADING(z/ 0,								L"(z / 0)",								void*);
	ASSERT_OVERLOADING(z% 0,								L"(z % 0)",								void*);
	ASSERT_OVERLOADING(z+ 0,								L"(z + 0)",								void*);
	ASSERT_OVERLOADING(z- 0,								L"(z - 0)",								void*);
	ASSERT_OVERLOADING(z<<0,								L"(z << 0)",							void*);
	ASSERT_OVERLOADING(z>>0,								L"(z >> 0)",							void*);
	ASSERT_OVERLOADING(z==0,								L"(z == 0)",							void*);
	ASSERT_OVERLOADING(z!=0,								L"(z != 0)",							void*);
	ASSERT_OVERLOADING(z< 0,								L"(z < 0)",								void*);
	ASSERT_OVERLOADING(z<=0,								L"(z <= 0)",							void*);
	ASSERT_OVERLOADING(z> 0,								L"(z > 0)",								void*);
	ASSERT_OVERLOADING(z>=0,								L"(z >= 0)",							void*);
	ASSERT_OVERLOADING(z& 0,								L"(z & 0)",								void*);
	ASSERT_OVERLOADING(z| 0,								L"(z | 0)",								void*);
	ASSERT_OVERLOADING(z^ 0,								L"(z ^ 0)",								void*);
	ASSERT_OVERLOADING(z&&0,								L"(z && 0)",							void*);
	ASSERT_OVERLOADING(z||0,								L"(z || 0)",							void*);
	ASSERT_OVERLOADING(z*=0,								L"(z *= 0)",							void*);
	ASSERT_OVERLOADING(z/=0,								L"(z /= 0)",							void*);
	ASSERT_OVERLOADING(z%=0,								L"(z %= 0)",							void*);
	ASSERT_OVERLOADING(z+=0,								L"(z += 0)",							void*);
	ASSERT_OVERLOADING(z-=0,								L"(z -= 0)",							void*);
	ASSERT_OVERLOADING(z<<0,								L"(z << 0)",							void*);
	ASSERT_OVERLOADING(z>>0,								L"(z >> 0)",							void*);
	ASSERT_OVERLOADING(z&=0,								L"(z &= 0)",							void*);
	ASSERT_OVERLOADING(z|=0,								L"(z |= 0)",							void*);
	ASSERT_OVERLOADING(z^=0,								L"(z ^= 0)",							void*);

	ASSERT_OVERLOADING(0* z,								L"(0 * z)",								void*);
	ASSERT_OVERLOADING(0/ z,								L"(0 / z)",								void*);
	ASSERT_OVERLOADING(0% z,								L"(0 % z)",								void*);
	ASSERT_OVERLOADING(0+ z,								L"(0 + z)",								void*);
	ASSERT_OVERLOADING(0- z,								L"(0 - z)",								void*);
	ASSERT_OVERLOADING(0<<z,								L"(0 << z)",							void*);
	ASSERT_OVERLOADING(0>>z,								L"(0 >> z)",							void*);
	ASSERT_OVERLOADING(0==z,								L"(0 == z)",							void*);
	ASSERT_OVERLOADING(0!=z,								L"(0 != z)",							void*);
	ASSERT_OVERLOADING(0< z,								L"(0 < z)",								void*);
	ASSERT_OVERLOADING(0<=z,								L"(0 <= z)",							void*);
	ASSERT_OVERLOADING(0> z,								L"(0 > z)",								void*);
	ASSERT_OVERLOADING(0>=z,								L"(0 >= z)",							void*);
	ASSERT_OVERLOADING(0& z,								L"(0 & z)",								void*);
	ASSERT_OVERLOADING(0| z,								L"(0 | z)",								void*);
	ASSERT_OVERLOADING(0^ z,								L"(0 ^ z)",								void*);
	ASSERT_OVERLOADING(0&&z,								L"(0 && z)",							void*);
	ASSERT_OVERLOADING(0||z,								L"(0 || z)",							void*);
	ASSERT_OVERLOADING(0*=z,								L"(0 *= z)",							void*);
	ASSERT_OVERLOADING(0/=z,								L"(0 /= z)",							void*);
	ASSERT_OVERLOADING(0%=z,								L"(0 %= z)",							void*);
	ASSERT_OVERLOADING(0+=z,								L"(0 += z)",							void*);
	ASSERT_OVERLOADING(0-=z,								L"(0 -= z)",							void*);
	ASSERT_OVERLOADING(0<<z,								L"(0 << z)",							void*);
	ASSERT_OVERLOADING(0>>z,								L"(0 >> z)",							void*);
	ASSERT_OVERLOADING(0&=z,								L"(0 &= z)",							void*);
	ASSERT_OVERLOADING(0|=z,								L"(0 |= z)",							void*);
	ASSERT_OVERLOADING(0^=z,								L"(0 ^= z)",							void*);

	ASSERT_OVERLOADING(cx* 0,								L"(cx * 0)",							bool*);
	ASSERT_OVERLOADING(cx/ 0,								L"(cx / 0)",							bool*);
	ASSERT_OVERLOADING(cx% 0,								L"(cx % 0)",							bool*);
	ASSERT_OVERLOADING(cx+ 0,								L"(cx + 0)",							bool*);
	ASSERT_OVERLOADING(cx- 0,								L"(cx - 0)",							bool*);
	ASSERT_OVERLOADING(cx<<0,								L"(cx << 0)",							bool*);
	ASSERT_OVERLOADING(cx>>0,								L"(cx >> 0)",							bool*);
	ASSERT_OVERLOADING(cx==0,								L"(cx == 0)",							bool*);
	ASSERT_OVERLOADING(cx!=0,								L"(cx != 0)",							bool*);
	ASSERT_OVERLOADING(cx< 0,								L"(cx < 0)",							bool*);
	ASSERT_OVERLOADING(cx<=0,								L"(cx <= 0)",							bool*);
	ASSERT_OVERLOADING(cx> 0,								L"(cx > 0)",							bool*);
	ASSERT_OVERLOADING(cx>=0,								L"(cx >= 0)",							bool*);
	ASSERT_OVERLOADING(cx& 0,								L"(cx & 0)",							bool*);
	ASSERT_OVERLOADING(cx| 0,								L"(cx | 0)",							bool*);
	ASSERT_OVERLOADING(cx^ 0,								L"(cx ^ 0)",							bool*);
	ASSERT_OVERLOADING(cx&&0,								L"(cx && 0)",							bool*);
	ASSERT_OVERLOADING(cx||0,								L"(cx || 0)",							bool*);
	ASSERT_OVERLOADING(cx= 0,								L"(cx = 0)",							bool*);
	ASSERT_OVERLOADING(cx*=0,								L"(cx *= 0)",							bool*);
	ASSERT_OVERLOADING(cx/=0,								L"(cx /= 0)",							bool*);
	ASSERT_OVERLOADING(cx%=0,								L"(cx %= 0)",							bool*);
	ASSERT_OVERLOADING(cx+=0,								L"(cx += 0)",							bool*);
	ASSERT_OVERLOADING(cx-=0,								L"(cx -= 0)",							bool*);
	ASSERT_OVERLOADING(cx<<0,								L"(cx << 0)",							bool*);
	ASSERT_OVERLOADING(cx>>0,								L"(cx >> 0)",							bool*);
	ASSERT_OVERLOADING(cx&=0,								L"(cx &= 0)",							bool*);
	ASSERT_OVERLOADING(cx|=0,								L"(cx |= 0)",							bool*);
	ASSERT_OVERLOADING(cx^=0,								L"(cx ^= 0)",							bool*);

	ASSERT_OVERLOADING(cy* 0,								L"(cy * 0)",							bool*);
	ASSERT_OVERLOADING(cy/ 0,								L"(cy / 0)",							bool*);
	ASSERT_OVERLOADING(cy% 0,								L"(cy % 0)",							bool*);
	ASSERT_OVERLOADING(cy+ 0,								L"(cy + 0)",							bool*);
	ASSERT_OVERLOADING(cy- 0,								L"(cy - 0)",							bool*);
	ASSERT_OVERLOADING(cy<<0,								L"(cy << 0)",							bool*);
	ASSERT_OVERLOADING(cy>>0,								L"(cy >> 0)",							bool*);
	ASSERT_OVERLOADING(cy==0,								L"(cy == 0)",							bool*);
	ASSERT_OVERLOADING(cy!=0,								L"(cy != 0)",							bool*);
	ASSERT_OVERLOADING(cy< 0,								L"(cy < 0)",							bool*);
	ASSERT_OVERLOADING(cy<=0,								L"(cy <= 0)",							bool*);
	ASSERT_OVERLOADING(cy> 0,								L"(cy > 0)",							bool*);
	ASSERT_OVERLOADING(cy>=0,								L"(cy >= 0)",							bool*);
	ASSERT_OVERLOADING(cy& 0,								L"(cy & 0)",							bool*);
	ASSERT_OVERLOADING(cy| 0,								L"(cy | 0)",							bool*);
	ASSERT_OVERLOADING(cy^ 0,								L"(cy ^ 0)",							bool*);
	ASSERT_OVERLOADING(cy&&0,								L"(cy && 0)",							bool*);
	ASSERT_OVERLOADING(cy||0,								L"(cy || 0)",							bool*);
	ASSERT_OVERLOADING(cy*=0,								L"(cy *= 0)",							bool*);
	ASSERT_OVERLOADING(cy/=0,								L"(cy /= 0)",							bool*);
	ASSERT_OVERLOADING(cy%=0,								L"(cy %= 0)",							bool*);
	ASSERT_OVERLOADING(cy+=0,								L"(cy += 0)",							bool*);
	ASSERT_OVERLOADING(cy-=0,								L"(cy -= 0)",							bool*);
	ASSERT_OVERLOADING(cy<<0,								L"(cy << 0)",							bool*);
	ASSERT_OVERLOADING(cy>>0,								L"(cy >> 0)",							bool*);
	ASSERT_OVERLOADING(cy&=0,								L"(cy &= 0)",							bool*);
	ASSERT_OVERLOADING(cy|=0,								L"(cy |= 0)",							bool*);
	ASSERT_OVERLOADING(cy^=0,								L"(cy ^= 0)",							bool*);

	ASSERT_OVERLOADING(0* cy,								L"(0 * cy)",							bool*);
	ASSERT_OVERLOADING(0/ cy,								L"(0 / cy)",							bool*);
	ASSERT_OVERLOADING(0% cy,								L"(0 % cy)",							bool*);
	ASSERT_OVERLOADING(0+ cy,								L"(0 + cy)",							bool*);
	ASSERT_OVERLOADING(0- cy,								L"(0 - cy)",							bool*);
	ASSERT_OVERLOADING(0<<cy,								L"(0 << cy)",							bool*);
	ASSERT_OVERLOADING(0>>cy,								L"(0 >> cy)",							bool*);
	ASSERT_OVERLOADING(0==cy,								L"(0 == cy)",							bool*);
	ASSERT_OVERLOADING(0!=cy,								L"(0 != cy)",							bool*);
	ASSERT_OVERLOADING(0< cy,								L"(0 < cy)",							bool*);
	ASSERT_OVERLOADING(0<=cy,								L"(0 <= cy)",							bool*);
	ASSERT_OVERLOADING(0> cy,								L"(0 > cy)",							bool*);
	ASSERT_OVERLOADING(0>=cy,								L"(0 >= cy)",							bool*);
	ASSERT_OVERLOADING(0& cy,								L"(0 & cy)",							bool*);
	ASSERT_OVERLOADING(0| cy,								L"(0 | cy)",							bool*);
	ASSERT_OVERLOADING(0^ cy,								L"(0 ^ cy)",							bool*);
	ASSERT_OVERLOADING(0&&cy,								L"(0 && cy)",							bool*);
	ASSERT_OVERLOADING(0||cy,								L"(0 || cy)",							bool*);
	ASSERT_OVERLOADING(0*=cy,								L"(0 *= cy)",							bool*);
	ASSERT_OVERLOADING(0/=cy,								L"(0 /= cy)",							bool*);
	ASSERT_OVERLOADING(0%=cy,								L"(0 %= cy)",							bool*);
	ASSERT_OVERLOADING(0+=cy,								L"(0 += cy)",							bool*);
	ASSERT_OVERLOADING(0-=cy,								L"(0 -= cy)",							bool*);
	ASSERT_OVERLOADING(0<<cy,								L"(0 << cy)",							bool*);
	ASSERT_OVERLOADING(0>>cy,								L"(0 >> cy)",							bool*);
	ASSERT_OVERLOADING(0&=cy,								L"(0 &= cy)",							bool*);
	ASSERT_OVERLOADING(0|=cy,								L"(0 |= cy)",							bool*);
	ASSERT_OVERLOADING(0^=cy,								L"(0 ^= cy)",							bool*);

	ASSERT_OVERLOADING(cz* 0,								L"(cz * 0)",							bool*);
	ASSERT_OVERLOADING(cz/ 0,								L"(cz / 0)",							bool*);
	ASSERT_OVERLOADING(cz% 0,								L"(cz % 0)",							bool*);
	ASSERT_OVERLOADING(cz+ 0,								L"(cz + 0)",							bool*);
	ASSERT_OVERLOADING(cz- 0,								L"(cz - 0)",							bool*);
	ASSERT_OVERLOADING(cz<<0,								L"(cz << 0)",							bool*);
	ASSERT_OVERLOADING(cz>>0,								L"(cz >> 0)",							bool*);
	ASSERT_OVERLOADING(cz==0,								L"(cz == 0)",							bool*);
	ASSERT_OVERLOADING(cz!=0,								L"(cz != 0)",							bool*);
	ASSERT_OVERLOADING(cz< 0,								L"(cz < 0)",							bool*);
	ASSERT_OVERLOADING(cz<=0,								L"(cz <= 0)",							bool*);
	ASSERT_OVERLOADING(cz> 0,								L"(cz > 0)",							bool*);
	ASSERT_OVERLOADING(cz>=0,								L"(cz >= 0)",							bool*);
	ASSERT_OVERLOADING(cz& 0,								L"(cz & 0)",							bool*);
	ASSERT_OVERLOADING(cz| 0,								L"(cz | 0)",							bool*);
	ASSERT_OVERLOADING(cz^ 0,								L"(cz ^ 0)",							bool*);
	ASSERT_OVERLOADING(cz&&0,								L"(cz && 0)",							bool*);
	ASSERT_OVERLOADING(cz||0,								L"(cz || 0)",							bool*);
	ASSERT_OVERLOADING(cz*=0,								L"(cz *= 0)",							bool*);
	ASSERT_OVERLOADING(cz/=0,								L"(cz /= 0)",							bool*);
	ASSERT_OVERLOADING(cz%=0,								L"(cz %= 0)",							bool*);
	ASSERT_OVERLOADING(cz+=0,								L"(cz += 0)",							bool*);
	ASSERT_OVERLOADING(cz-=0,								L"(cz -= 0)",							bool*);
	ASSERT_OVERLOADING(cz<<0,								L"(cz << 0)",							bool*);
	ASSERT_OVERLOADING(cz>>0,								L"(cz >> 0)",							bool*);
	ASSERT_OVERLOADING(cz&=0,								L"(cz &= 0)",							bool*);
	ASSERT_OVERLOADING(cz|=0,								L"(cz |= 0)",							bool*);
	ASSERT_OVERLOADING(cz^=0,								L"(cz ^= 0)",							bool*);

	ASSERT_OVERLOADING(0* cz,								L"(0 * cz)",							bool*);
	ASSERT_OVERLOADING(0/ cz,								L"(0 / cz)",							bool*);
	ASSERT_OVERLOADING(0% cz,								L"(0 % cz)",							bool*);
	ASSERT_OVERLOADING(0+ cz,								L"(0 + cz)",							bool*);
	ASSERT_OVERLOADING(0- cz,								L"(0 - cz)",							bool*);
	ASSERT_OVERLOADING(0<<cz,								L"(0 << cz)",							bool*);
	ASSERT_OVERLOADING(0>>cz,								L"(0 >> cz)",							bool*);
	ASSERT_OVERLOADING(0==cz,								L"(0 == cz)",							bool*);
	ASSERT_OVERLOADING(0!=cz,								L"(0 != cz)",							bool*);
	ASSERT_OVERLOADING(0< cz,								L"(0 < cz)",							bool*);
	ASSERT_OVERLOADING(0<=cz,								L"(0 <= cz)",							bool*);
	ASSERT_OVERLOADING(0> cz,								L"(0 > cz)",							bool*);
	ASSERT_OVERLOADING(0>=cz,								L"(0 >= cz)",							bool*);
	ASSERT_OVERLOADING(0& cz,								L"(0 & cz)",							bool*);
	ASSERT_OVERLOADING(0| cz,								L"(0 | cz)",							bool*);
	ASSERT_OVERLOADING(0^ cz,								L"(0 ^ cz)",							bool*);
	ASSERT_OVERLOADING(0&&cz,								L"(0 && cz)",							bool*);
	ASSERT_OVERLOADING(0||cz,								L"(0 || cz)",							bool*);
	ASSERT_OVERLOADING(0*=cz,								L"(0 *= cz)",							bool*);
	ASSERT_OVERLOADING(0/=cz,								L"(0 /= cz)",							bool*);
	ASSERT_OVERLOADING(0%=cz,								L"(0 %= cz)",							bool*);
	ASSERT_OVERLOADING(0+=cz,								L"(0 += cz)",							bool*);
	ASSERT_OVERLOADING(0-=cz,								L"(0 -= cz)",							bool*);
	ASSERT_OVERLOADING(0<<cz,								L"(0 << cz)",							bool*);
	ASSERT_OVERLOADING(0>>cz,								L"(0 >> cz)",							bool*);
	ASSERT_OVERLOADING(0&=cz,								L"(0 &= cz)",							bool*);
	ASSERT_OVERLOADING(0|=cz,								L"(0 |= cz)",							bool*);
	ASSERT_OVERLOADING(0^=cz,								L"(0 ^= cz)",							bool*);
}

#undef ASSERT_OVERLOADING

#pragma warning (pop)