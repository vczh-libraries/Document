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