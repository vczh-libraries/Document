#include <type_traits>
#include "Util.h"
#include "..\Core\Source\Ast_Decl.h"
#include "..\Core\Source\Ast_Type.h"
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
	static void Assert(const WString& name, const WString& functionName, CppReferenceType parameterRefType, ParsingArguments& pa)
	{
		auto classSymbol = pa.context->children[L"test_generated_functions"][0]->children[name][0].Obj();
		vint index = classSymbol->children.Keys().IndexOf(functionName);
		if (index != -1)
		{
			auto& funcs = classSymbol->children.GetByIndex(index);
			for (vint i = 0; i < funcs.Count(); i++)
			{
				auto funcDecl = funcs[i]->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
				if (funcType->parameters.Count() == 0)
				{
					if (parameterRefType == CppReferenceType::Ptr)
					{
						if (funcDecl->decoratorDefault) TEST_ASSERT(Ability);
						if (funcDecl->decoratorDelete) TEST_ASSERT(!Ability);
						return;
					}
				}
				else if (funcType->parameters.Count() == 1)
				{
					if (auto refType = funcType->parameters[0]->type.Cast<ReferenceType>())
					{
						if (refType->reference == parameterRefType)
						{
							if (funcDecl->decoratorDefault) TEST_ASSERT(Ability);
							if (funcDecl->decoratorDelete) TEST_ASSERT(!Ability);
							return;
						}
					}
				}
			}
		}
		TEST_ASSERT(!Ability);
	}

	static void DefaultCtor(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"$__ctor", CppReferenceType::Ptr, pa);
	}

	static void CopyCtor(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"$__ctor", CppReferenceType::LRef, pa);
	}

	static void MoveCtor(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"$__ctor", CppReferenceType::RRef, pa);
	}

	static void CopyAssign(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"operator =", CppReferenceType::LRef, pa);
	}

	static void MoveAssign(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"operator =", CppReferenceType::RRef, pa);
	}

	static void DefaultDtor(const WString& name, ParsingArguments& pa)
	{
		Assert(name, L"~" + name, CppReferenceType::Ptr, pa);
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