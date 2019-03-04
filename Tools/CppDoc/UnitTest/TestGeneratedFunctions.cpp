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
#define SYMBOL(KIND) GetSpecialMember(pa, classSymbol, SpecialMemberKind::KIND)
#define DEFINED(KIND) (SYMBOL(KIND) != nullptr)
#define DELETED(KIND) ([&](){auto symbol = SYMBOL(KIND); return symbol && symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>()->decoratorDelete; }())
#define ENABLED(KIND) IsSpecialMemberEnabled(SYMBOL(KIND))

	static void DefaultCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == (ENABLED(DefaultCtor) && ENABLED(Dtor)));
	}

	static void CopyCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == (ENABLED(CopyCtor) && ENABLED(Dtor)));
	}

	static void MoveCtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == ((ENABLED(CopyCtor) || ENABLED(MoveCtor)) && !DELETED(MoveCtor) && ENABLED(Dtor)));
	}

	static void CopyAssign(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == ENABLED(CopyAssignOp));
	}

	static void MoveAssign(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		auto opEnabled = IsSpecialMemberEnabled(GetSpecialMember(pa, classSymbol, SpecialMemberKind::MoveAssignOp));
		TEST_ASSERT(Ability == ((ENABLED(CopyAssignOp) || ENABLED(MoveAssignOp)) &&  !DELETED(MoveAssignOp)));
	}

	static void DefaultDtor(const WString& name, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		TEST_ASSERT(Ability == ENABLED(Dtor));
	}

#undef SYMBOL
#undef DEFINED
#undef DELETED
#undef ENABLED
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