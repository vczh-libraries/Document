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
		ASSERT_SYMBOL(0, L"X", 4, 7, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(1, L"F", 7, 8, ForwardFunctionDeclaration, 6, 4)
		ASSERT_SYMBOL(2, L"x", 7, 10, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(3, L"X", 7, 13, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL_OVERLOAD(4, L"F", 7, 8, ForwardFunctionDeclaration, 4, 5)
	END_ASSERT_SYMBOL;

	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	TEST_ASSERT(accessed.Count() == 5);
}