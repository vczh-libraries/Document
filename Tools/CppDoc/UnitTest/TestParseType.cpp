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
}