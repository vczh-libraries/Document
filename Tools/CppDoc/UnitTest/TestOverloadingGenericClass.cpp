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
	
		ASSERT_OVERLOADING_SIMPLE(F(cd),					double *);
		ASSERT_OVERLOADING_SIMPLE(F(d),						bool *);
		ASSERT_OVERLOADING_SIMPLE(F(Derived<void>()),		char *);
	
		ASSERT_OVERLOADING_SIMPLE(G(cd),					double);
		ASSERT_OVERLOADING_SIMPLE(G(d),						bool);
		ASSERT_OVERLOADING_SIMPLE(G(Derived<void>()),		char);
	});

	TEST_CATEGORY(L"Constructors")
	{
		using namespace Input__TestOverloadingGenericClass_Overloading_Constructor;
		COMPILE_PROGRAM(program, pa, input);
	
		ASSERT_OVERLOADING_SIMPLE(F(1),						bool);
		ASSERT_OVERLOADING_SIMPLE(F(1.0),					bool);
		ASSERT_OVERLOADING_SIMPLE(F("x"),					char *);
		ASSERT_OVERLOADING_SIMPLE(F(L"x"),					char *);
	});

	TEST_CATEGORY(L"Operators")
	{
		using namespace Input__TestOverloadingGenericClass_Overloading_OperatorOverloading;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(F(X<void *>()),			bool);
		ASSERT_OVERLOADING_SIMPLE(F(X<char *>()),			double);
		ASSERT_OVERLOADING_SIMPLE(F(X<wchar_t *>()),		float);
	});
}