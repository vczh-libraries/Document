#include <type_traits>
#include "Util.h"
#include "..\Core\Source\Ast.h"
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
		auto ctorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::DefaultCtor));
		auto dtorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::Dtor));
		TEST_ASSERT(Ability == (ctorEnabled && dtorEnabled));
	}

	static void CopyCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto ctorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::CopyCtor));
		auto dtorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::Dtor));
		TEST_ASSERT(Ability == (ctorEnabled && dtorEnabled));
	}

	static void MoveCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto ctorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::MoveCtor));
		auto dtorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::Dtor));
		TEST_ASSERT(Ability == (ctorEnabled && dtorEnabled));
	}

	static void CopyAssign(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto opEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::CopyAssignOp));
		TEST_ASSERT(Ability == opEnabled);
	}

	static void MoveAssign(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto opEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::MoveAssignOp));
		TEST_ASSERT(Ability == opEnabled);
	}

	static void DefaultDtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto dtorEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::Dtor));
		TEST_ASSERT(Ability == dtorEnabled);
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