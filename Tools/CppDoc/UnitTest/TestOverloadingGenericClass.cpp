#include "Util.h"

namespace Input__TestOverloadingGenericClass_Overloading_BaseClass
{
	TEST_DECL(
		template<typename T>
		struct Base {};

		template<typename T>
		struct Derived : Base<T*>, Base<T> {};

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

namespace Input__TestOverloadingGenericClass_Overloading_Constructor
{
	TEST_DECL(
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

namespace Input__TestOverloadingGenericClass_Overloading_OperatorOverloading
{
	TEST_DECL(
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

TEST_FILE
{
	TEST_CATEGORY(L"Base classes")
	{
		using namespace Input__TestOverloadingGenericClass_Overloading_BaseClass;
		COMPILE_PROGRAM(program, pa, input);
	
		ASSERT_OVERLOADING(F(cd),					L"F(cd)",					double *);
		ASSERT_OVERLOADING(F(d),					L"F(d)",					bool *);
		ASSERT_OVERLOADING(F(Derived<void>()),		L"F(Derived<void>())",		char *);
	
		ASSERT_OVERLOADING(G(cd),					L"G(cd)",					double);
		ASSERT_OVERLOADING(G(d),					L"G(d)",					bool);
		ASSERT_OVERLOADING(G(Derived<void>()),		L"G(Derived<void>())",		char);
	});

	TEST_CATEGORY(L"Constructors")
	{
		using namespace Input__TestOverloadingGenericClass_Overloading_Constructor;
		COMPILE_PROGRAM(program, pa, input);
	
		ASSERT_OVERLOADING(F(1),					L"F(1)",					bool);
		ASSERT_OVERLOADING(F(1.0),					L"F(1.0)",					bool);
		ASSERT_OVERLOADING(F("x"),					L"F(\"x\")",				char *);
		ASSERT_OVERLOADING(F(L"x"),					L"F(L\"x\")",				char *);
	});

	TEST_CATEGORY(L"Operators")
	{
		using namespace Input__TestOverloadingGenericClass_Overloading_OperatorOverloading;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(F(X<void*>()),			L"F(X<void *>())",			bool);
		ASSERT_OVERLOADING(F(X<char*>()),			L"F(X<char *>())",			double);
		ASSERT_OVERLOADING(F(X<wchar_t*>()),		L"F(X<wchar_t *>())",		float);
	});
}