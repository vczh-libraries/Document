#include <type_traits>
#include "Util.h"
#include "..\Core\Source\Ast_Decl.h"
#include "TestGeneratedFunctions_Input.h"
#include "TestGeneratedFunctions_Macro.h"

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
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::DefaultCtor));
	}

	static void CopyCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::CopyCtor));
	}

	static void MoveCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::MoveCtor));
	}

	static void CopyAssignOp(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::CopyAssignOp));
	}

	static void MoveAssignOp(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::MoveAssignOp));
	}

	static void Dtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == IsSpecialMemberFeatureEnabled(pa, classSymbol, SpecialMemberKind::Dtor));
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

#define FEATURE(TYPE) TestGC_Helper<std::is_copy_assignable_v<TYPE>>::CopyAssignOp(L#TYPE, pa);
	GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_move_assignable_v<TYPE>>::MoveAssignOp(L#TYPE, pa);
	GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE

#define FEATURE(TYPE) TestGC_Helper<std::is_destructible_v<TYPE>>::Dtor(L#TYPE, pa);
	GENERATED_FUNCTION_TYPES(FEATURE)
#undef FEATURE
}