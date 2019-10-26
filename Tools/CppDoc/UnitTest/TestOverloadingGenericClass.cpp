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

TEST_CASE(TestParseGenericClass_Overloading_SpecialMember)
{
}

TEST_CASE(TestParseGenericClass_Overloading_GeneratedSpecialMember)
{
}

TEST_CASE(TestParseGenericClass_Overloading_OperatorOverloading)
{
}

#undef _