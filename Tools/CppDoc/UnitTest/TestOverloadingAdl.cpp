#include <Ast_Decl.h>
#include "Util.h"

#include "TestOverloadingAdl_Input.h"

WString LoadOverloadingAdlCode()
{
	FilePath input = L"../UnitTest/TestOverloadingAdl_Input.h";

	WString code;
	TEST_ASSERT(File(input).ReadAllTextByBom(code));

	return code;
}

TEST_CASE(TestParseExpr_Overloading_ADL)
{
	COMPILE_PROGRAM(program, pa, LoadOverloadingAdlCode().Buffer());

	ASSERT_OVERLOADING(F(test_overloading_adl::y::C::D()),			L"F(test_overloading_adl :: y :: C :: D())",				double);
	ASSERT_OVERLOADING(F(test_overloading_adl::z::E::F()),			L"F(test_overloading_adl :: z :: E :: F())",				bool);

	ASSERT_OVERLOADING(pF(test_overloading_adl::pFA),				L"pF(test_overloading_adl :: pFA)",							void);
	ASSERT_OVERLOADING(pF(test_overloading_adl::pFB),				L"pF(test_overloading_adl :: pFB)",							void);
	ASSERT_OVERLOADING(pF(test_overloading_adl::pFC),				L"pF(test_overloading_adl :: pFC)",							void);
	ASSERT_OVERLOADING(pF(test_overloading_adl::pFD),				L"pF(test_overloading_adl :: pFD)",							void);
	ASSERT_OVERLOADING(pF(test_overloading_adl::pFE),				L"pF(test_overloading_adl :: pFE)",							void);
	ASSERT_OVERLOADING(pF(test_overloading_adl::pFF),				L"pF(test_overloading_adl :: pFF)",							void);

	ASSERT_OVERLOADING(pG(&test_overloading_adl::x::A::w),			L"pG((& test_overloading_adl :: x :: A :: w))",				float);
	ASSERT_OVERLOADING(pG(&test_overloading_adl::x::A::B::w),		L"pG((& test_overloading_adl :: x :: A :: B :: w))",		float);
	ASSERT_OVERLOADING(pG(&test_overloading_adl::y::C::w),			L"pG((& test_overloading_adl :: y :: C :: w))",				float);
	ASSERT_OVERLOADING(pG(&test_overloading_adl::y::C::D::w),		L"pG((& test_overloading_adl :: y :: C :: D :: w))",		float);
	ASSERT_OVERLOADING(pG(&test_overloading_adl::z::E::w),			L"pG((& test_overloading_adl :: z :: E :: w))",				float);
	ASSERT_OVERLOADING(pG(&test_overloading_adl::z::E::F::w),		L"pG((& test_overloading_adl :: z :: E :: F :: w))",		float);
}

TEST_CASE(TestParseExpr_Overloading_ADL_ReIndexFunc)
{
	auto input = LR"(
namespace x
{
	struct X{};
	int F(X);
}
int F(int);
int f = F(x::X());
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL			(0, L"X", 4, 7, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(1, L"F", 7, 8, ForwardFunctionDeclaration, 6, 4)
		ASSERT_SYMBOL			(2, L"x", 7, 10, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL			(3, L"X", 7, 13, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL_OVERLOAD	(4, L"F", 7, 8, ForwardFunctionDeclaration, 4, 5)
	END_ASSERT_SYMBOL;

	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	TEST_ASSERT(accessed.Count() == 5);
}

TEST_CASE(TestParseExpr_Overloading_ADL_ReIndexOperator)
{
	auto input = LR"(
namespace x
{
	struct X{};
	int operator++(X);
	int operator++(X, int);
	int operator+(X, int);
}
struct Y{};
int operator++(Y);
int operator++(Y, int);
int operator+(Y, int);
int a = x::X()++;
int b = ++x::X();
int c = x::X()+1;
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL			(0, L"X", 4, 16, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(1, L"X", 5, 16, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(2, L"X", 6, 15, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(3, L"Y", 9, 15, ClassDeclaration, 8, 7)
		ASSERT_SYMBOL			(4, L"Y", 10, 15, ClassDeclaration, 8, 7)
		ASSERT_SYMBOL			(5, L"Y", 11, 14, ClassDeclaration, 8, 7)
		ASSERT_SYMBOL			(6, L"x", 12, 8, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL			(7, L"X", 12, 11, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(8, L"x", 13, 10, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL			(9, L"X", 13, 13, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL			(10, L"x", 14, 8, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL			(11, L"X", 14, 11, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL_OVERLOAD	(12, L"++", 12, 14, ForwardFunctionDeclaration, 5, 5)
		ASSERT_SYMBOL_OVERLOAD	(13, L"++", 13, 8, ForwardFunctionDeclaration, 4, 5)
		ASSERT_SYMBOL_OVERLOAD	(14, L"+", 14, 14, ForwardFunctionDeclaration, 6, 5)
	END_ASSERT_SYMBOL;

	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	TEST_ASSERT(accessed.Count() == 15);
}

TEST_CASE(TestParseExpr_Overloading_ADL_ReIndexOperator2)
{
	auto input = LR"(
struct X
{
	int x;
	int operator()(int);
	int operator[](int);
	X* operator->();
};
int a = X()(0);
int b = X()[0];
int c = X()->x;
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL			(0, L"X", 6, 1, ClassDeclaration, 1, 7)
		ASSERT_SYMBOL			(1, L"X", 8, 8, ClassDeclaration, 1, 7)
		ASSERT_SYMBOL			(2, L"X", 9, 8, ClassDeclaration, 1, 7)
		ASSERT_SYMBOL			(3, L"X", 10, 8, ClassDeclaration, 1, 7)
		ASSERT_SYMBOL			(4, L"x", 10, 13, VariableDeclaration, 3, 5)
		ASSERT_SYMBOL_OVERLOAD	(5, L"()", 8, 11, ForwardFunctionDeclaration, 4, 5)
		ASSERT_SYMBOL_OVERLOAD	(6, L"[]", 9, 11, ForwardFunctionDeclaration, 5, 5)
		ASSERT_SYMBOL_OVERLOAD	(7, L"->", 10, 11, ForwardFunctionDeclaration, 6, 4)
	END_ASSERT_SYMBOL;

	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	TEST_ASSERT(accessed.Count() == 8);
}