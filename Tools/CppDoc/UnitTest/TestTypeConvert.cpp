#include <Parser.h>
#include "Util.h"

void AssertTypeConvert(ParsingArguments& pa, const WString fromCppType, const WString& toCppType, TsysConv conv)
{
	TypeTsysList fromTypes, toTypes;
	Ptr<Type> fromType, toType;
	{
		CppTokenReader reader(GlobalCppLexer(), fromCppType);
		auto cursor = reader.GetFirstToken();
		fromType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}
	{
		CppTokenReader reader(GlobalCppLexer(), toCppType);
		auto cursor = reader.GetFirstToken();
		toType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}

	TypeToTsys(pa, fromType, fromTypes);
	TypeToTsys(pa, toType, toTypes);
	TEST_ASSERT(fromTypes.Count() == 1);
	TEST_ASSERT(toTypes.Count() == 1);

	auto output = TestConvert(toTypes[0], fromTypes[0]);
	TEST_ASSERT(output == conv);
}

void AssertTypeConvert(const WString fromCppType, const WString& toCppType, TsysConv conv)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
	AssertTypeConvert(pa, fromCppType, toCppType, conv);
}

TEST_CASE(TestTypeConvert_Exact)
{
}

TEST_CASE(TestTypeConvert_TrivalConversion)
{
}

TEST_CASE(TestTypeConvert_IntegralPromotion)
{
}

TEST_CASE(TestTypeConvert_StandardConversion)
{
}

TEST_CASE(TestTypeConvert_UserDefinedConversion)
{
}

TEST_CASE(TestTypeConvert_Illegal)
{
}