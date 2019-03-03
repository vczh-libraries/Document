#include "Util.h"
#include "TestGeneratedFunctions_Input.h"
#include "TestGeneratedFunctions_Macro.h"
#include <type_traits>

using namespace std;

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
	static void DefaultCtor(const WString& name, const ParsingArguments& pa)
	{
		WString code = name + L"()";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, code, TestGC_TypeSelector<Ability>::GetType(type), pa);
	}

	static void CopyCtor(const WString& name, const ParsingArguments& pa)
	{
		WString code = name + L"(static_cast<" + name + L" const &>(nullptr))";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, code, TestGC_TypeSelector<Ability>::GetType(type), pa);
	}

	static void MoveCtor(const WString& name, const ParsingArguments& pa)
	{
		WString code = name + L"(static_cast<" + name + L" &&>(nullptr))";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, code, TestGC_TypeSelector<Ability>::GetType(type), pa);
	}

	static void CopyAssign(const WString& name, const ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr) = " + L"static_cast<" + name + L" const &>(nullptr)";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, L"(" + code + L")", TestGC_TypeSelector<Ability>::GetType(type + L" &"), pa);
	}

	static void MoveAssign(const WString& name, const ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr) = " + L"static_cast<" + name + L" &&>(nullptr)";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, L"(" + code + L")", TestGC_TypeSelector<Ability>::GetType(type + L" &"), pa);
	}

	static void DefaultConstructible(const WString& name, const ParsingArguments& pa)
	{
		WString code = L"static_cast<" + name + L" &>(nullptr).~" + name + L"()";
		WString type = L"::test_generated_functions::" + name;
		AssertExpr(code, L"(" + code + L")", TestGC_TypeSelector<Ability>::GetType(type + L" &"), pa);
	}
};

TEST_CASE(TestGF_Features)
{
	COMPILE_PROGRAM(program, pa, LoadGeneratedFunctionsCode().Buffer());
}