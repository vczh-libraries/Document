#include <Parser.h>
#include <Ast_Type.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);

void AssertType(const WString& type, const WString& log)
{
	CppTokenReader reader(GlobalCppLexer(), type);
	auto cursor = reader.GetFirstToken();

	ParsingArguments pa;
	auto decorator = ParseDeclarator(pa, DecoratorRestriction::Zero, InitializerRestriction::Zero, cursor);
	TEST_ASSERT(!cursor);


	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(decorator->type, writer);
	});
	TEST_ASSERT(output == log);
}

TEST_CASE(TestParseType_Primitive)
{
#define TEST_PRIMITIVE_TYPE(TYPE)\
	AssertType(L#TYPE, L#TYPE);\
	AssertType(L"signed " L#TYPE, L"signed " L#TYPE);\
	AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE);\

	TEST_PRIMITIVE_TYPE(auto);
	TEST_PRIMITIVE_TYPE(void);
	TEST_PRIMITIVE_TYPE(bool);
	TEST_PRIMITIVE_TYPE(char);
	TEST_PRIMITIVE_TYPE(wchar_t);
	TEST_PRIMITIVE_TYPE(char16_t);
	TEST_PRIMITIVE_TYPE(char32_t);
	TEST_PRIMITIVE_TYPE(short);
	TEST_PRIMITIVE_TYPE(int);
	TEST_PRIMITIVE_TYPE(__int8);
	TEST_PRIMITIVE_TYPE(__int16);
	TEST_PRIMITIVE_TYPE(__int32);
	TEST_PRIMITIVE_TYPE(__int64);
	TEST_PRIMITIVE_TYPE(long);
	TEST_PRIMITIVE_TYPE(long long);
	TEST_PRIMITIVE_TYPE(float);
	TEST_PRIMITIVE_TYPE(double);
	TEST_PRIMITIVE_TYPE(long double);

#undef TEST_PRIMITIVE_TYPE
}

TEST_CASE(TestParseType_Short)
{
}

TEST_CASE(TestParseType_Long)
{
}