#include <Parser.h>
#include "Util.h"

TEST_CASE(TestTypeConvertGeneric)
{
	auto contextInput = LR"(
struct S{};

template<typename T, T Value>
using Context = T;
)";
	COMPILE_PROGRAM(program, pa, contextInput);

	const wchar_t* typeCodes[] = {
		L"T",
		L"const T",
		L"volatile T",
		L"const volatile T",

		L"T*",
		L"const T*",
		L"volatile T*",
		L"const volatile T*",

		L"T&",
		L"const T&",
		L"volatile T&",
		L"const volatile T&",

		L"T&&",
		L"const T&&",
		L"volatile T&&",
		L"const volatile T&&",

		L"T[10]",
		L"const T[10]",
		L"volatile T[10]",
		L"const volatile T[10]",

		L"T(*)[10]",
		L"const T(*)[10]",
		L"volatile T(*)[10]",
		L"const volatile T(*)[10]",

		L"T(&)[10]",
		L"const T(&)[10]",
		L"volatile T(&)[10]",
		L"const volatile T(&)[10]",

		L"T(&&)[10]",
		L"const T(&&)[10]",
		L"volatile T(&&)[10]",
		L"const volatile T(&&)[10]",

		L"T S::*",
		L"S T::*",
		L"decltype({Value})",
		L"decltype({Value,S()})",
		L"decltype({S(),Value})",
		L"T(*)(S)",
		L"S(*)(T)",
		L"T(*)()",
		L"T(*[10])(S)",
		L"S(*[10])(T)",
		L"T(*[10])()",
		L"T(*&)(S)",
		L"S(*&)(T)",
		L"T(*&)()",
		L"T(*&&)(S)",
		L"S(*&&)(T)",
		L"T(*&&)()",
	};

	const int TypeCount = sizeof(typeCodes) / sizeof(*typeCodes);
	ITsys* genericTypes[TypeCount];
	ITsys* intTypes[TypeCount];
	ITsys* structTypes[TypeCount];

	auto contextSymbol = pa.context->children[L"Context"][0].Obj();
	auto spa = pa.WithContext(contextSymbol);
	{
		for (vint i = 0; i < TypeCount; i++)
		{
			TOKEN_READER(typeCodes[i]);
			auto cursor = reader.GetFirstToken();
			auto type = ParseType(spa, cursor);
			TEST_ASSERT(!cursor);

			TypeTsysList tsys;
			TypeToTsys(spa, type, tsys, nullptr);
			TEST_ASSERT(tsys.Count() == 1);
			genericTypes[i] = tsys[0];
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			TypeTsysList tsys;
			GenericArgContext gaContext;
			gaContext.arguments.Add(genericTypes[0], pa.tsys->Int());
			genericTypes[i]->ReplaceGenericArgs(gaContext, tsys);
			TEST_ASSERT(tsys.Count() == 1);
			intTypes[i] = tsys[0];
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			TypeTsysList tsys;
			GenericArgContext gaContext;
			gaContext.arguments.Add(genericTypes[0], pa.tsys->DeclOf(pa.context->children[L"S"][0].Obj()));
			genericTypes[i]->ReplaceGenericArgs(gaContext, tsys);
			TEST_ASSERT(tsys.Count() == 1);
			structTypes[i] = tsys[0];
		}
	}

	for (vint i = 0; i < TypeCount; i++)
	{
		auto fromType = genericTypes[i];
		for (vint j = 0; j < TypeCount; j++)
		{
			{
				auto toType = intTypes[j];
			}
			{
				auto toType = structTypes[j];
			}
		}
	}

	for (vint i = 0; i < TypeCount; i++)
	{
		{
			auto fromType = intTypes[i];
			for (vint j = 0; j < TypeCount; j++)
			{
				auto toType = genericTypes[j];
			}
		}
		{
			auto fromType = structTypes[i];
			for (vint j = 0; j < TypeCount; j++)
			{
				auto toType = genericTypes[j];
			}
		}
	}

	for (vint i = 0; i < TypeCount; i++)
	{
		auto fromType = genericTypes[i];
		for (vint j = 0; j < TypeCount; j++)
		{
			auto toType = genericTypes[j];
		}
	}

	for (vint i = 0; i < TypeCount; i++)
	{
		auto fromType = genericTypes[i];
		auto toType = pa.tsys->Any();
		auto result = TestConvert(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
		TEST_ASSERT(result == TsysConv::Any);
	}

	for (vint i = 0; i < TypeCount; i++)
	{
		auto fromType = pa.tsys->Any();
		auto toType = genericTypes[i];
		auto result = TestConvert(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
		TEST_ASSERT(result == TsysConv::Any);
	}
}