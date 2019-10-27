#include "Util.h"

#define _ ,

namespace Input__TestParseGenericClass_Overloading_BaseClass
{
	TEST_DECL_NO_REFINE(
		template<typename T>
		struct Base {};

		template<typename T>
		struct Derived : Base<T*> _ Base<T> {};

		double* F(const Base<void*>&);
		bool* F(Base<void*>&);
		char* F(Base<void*>&&);

		double G(const Base<void>&);
		bool G(Base<void>&);
		char G(Base<void>&&);

		Derived<void> d;
		const Derived<void> cd;
	);
}

TEST_CASE(TestParseGenericClass_Overloading_BaseClass)
{
	using namespace Input__TestParseGenericClass_Overloading_BaseClass;
	RefineInput(input);
	COMPILE_PROGRAM(program, pa, input);
	
	ASSERT_OVERLOADING(F(cd),					L"F(cd)",					double *);
	ASSERT_OVERLOADING(F(d),					L"F(d)",					bool *);
	ASSERT_OVERLOADING(F(Derived<void>()),		L"F(Derived<void>())",		char *);
	
	ASSERT_OVERLOADING(G(cd),					L"G(cd)",					double);
	ASSERT_OVERLOADING(G(d),					L"G(d)",					bool);
	ASSERT_OVERLOADING(G(Derived<void>()),		L"G(Derived<void>())",		char);
}

namespace Input__TestParseGenericClass_Overloading_Constructor
{
	TEST_DECL_NO_REFINE(
		template<typename T>
		struct X
		{
			X(int);
			X(T);
		};

		template<typename T>
		struct Y
		{
			Y(const char*);
			Y(T);
		};

		bool F(X<double>);
		char* F(Y<const wchar_t*>);
	);
}

TEST_CASE(TestParseGenericClass_Overloading_Constructor)
{
	using namespace Input__TestParseGenericClass_Overloading_Constructor;
	RefineInput(input);
	COMPILE_PROGRAM(program, pa, input);
	
	ASSERT_OVERLOADING(F(1),					L"F(1)",					bool);
	ASSERT_OVERLOADING(F(1.0),					L"F(1.0)",					bool);
	ASSERT_OVERLOADING(F("x"),					L"F(\"x\")",				char *);
	ASSERT_OVERLOADING(F(L"x"),					L"F(L\"x\")",				char *);
}

namespace Input__TestParseGenericClass_Overloading_OperatorOverloading
{
	TEST_DECL_NO_REFINE(
		template<typename T>
		struct X
		{
			operator int();
			operator T();
		};

		bool F(...);
		double F(char*);
		float F(wchar_t*);
	);
}

TEST_CASE(TestParseGenericClass_Overloading_OperatorOverloading)
{
	using namespace Input__TestParseGenericClass_Overloading_OperatorOverloading;
	RefineInput(input);
	COMPILE_PROGRAM(program, pa, input);

	ASSERT_OVERLOADING(F(X<void*>()),			L"F(X<void *>())",			bool);
	ASSERT_OVERLOADING(F(X<char*>()),			L"F(X<char *>())",			double);
	ASSERT_OVERLOADING(F(X<wchar_t*>()),		L"F(X<wchar_t *>())",		float);
}

#undef _