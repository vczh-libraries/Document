#include "Util.h"
#include "TestGeneratedFunctions_Input.h"
#include "TestGeneratedFunctions_Macro.h"
#include <type_traits>

WString LoadGeneratedFunctionsCode()
{
	FilePath input = L"../UnitTest/TestGeneratedFunctions_Input.h";

	WString code;
	TEST_ASSERT(File(input).ReadAllTextByBom(code));

	return code;
}

template<bool Ability>
struct TestGC_TypeSelector
{
	static WString GetType(const WString& type)
	{
		return type + L" $PR";
	}
};

template<>
struct TestGC_TypeSelector<false>
{
	static WString GetType(const WString& type)
	{
		return {};
	}
};

template<bool Ability>
struct TestGC_Helper
{
	static void DefaultCtor(const WString& name, ParsingArguments& pa)
	{
		WString code = name + L"()";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), code.Buffer(), TestGC_TypeSelector<Ability>::GetType(type).Buffer(), pa);
	}

	static void CopyCtor(const WString& name, ParsingArguments& pa)
	{
		WString code = name + L"(static_cast<" + name + L" const &>(nullptr))";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), code.Buffer(), TestGC_TypeSelector<Ability>::GetType(type).Buffer(), pa);
	}

	static void MoveCtor(const WString& name, ParsingArguments& pa)
	{
		WString code = name + L"(static_cast<" + name + L" &&>(nullptr))";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), code.Buffer(), TestGC_TypeSelector<Ability>::GetType(type).Buffer(), pa);
	}

	static void CopyAssign(const WString& name, ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr) = " + L"static_cast<" + name + L" const &>(nullptr)";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), (L"(" + code + L")").Buffer(), TestGC_TypeSelector<Ability>::GetType(type + L" &").Buffer(), pa);
	}

	static void MoveAssign(const WString& name, ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr) = " + L"static_cast<" + name + L" &&>(nullptr)";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), (L"(" + code + L")").Buffer(), TestGC_TypeSelector<Ability>::GetType(type + L" &").Buffer(), pa);
	}

	static void DefaultDtor(const WString& name, ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr).~" + name + L"()";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code.Buffer(), code.Buffer(), TestGC_TypeSelector<Ability>::GetType(type + L" &").Buffer(), pa);
	}
};

TEST_CASE(TestGF_Features)
{
	COMPILE_PROGRAM(program, pa, LoadGeneratedFunctionsCode().Buffer());

#define FEATURE(TYPE) TestGC_Helper<std::is_default_constructible_v<TYPE>>::DefaultCtor(L#TYPE, pa);
	GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_copy_constructible_v<TYPE>>::CopyCtor(L#TYPE, pa);
		GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_move_constructible_v<TYPE>>::MoveCtor(L#TYPE, pa);
		GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_copy_assignable_v<TYPE>>::CopyAssign(L#TYPE, pa);
		GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_move_assignable_v<TYPE>>::MoveAssign(L#TYPE, pa);
		GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_destructible_v<TYPE>>::DefaultDtor(L#TYPE, pa);
		GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE
}