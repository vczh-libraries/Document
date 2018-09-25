#include <Parser.h>
#include <Ast_Decl.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer);
extern void					Log(Ptr<Program> program, StreamWriter& writer);

void AssertProgram(const WString& type, const WString& log)
{
	CppTokenReader reader(GlobalCppLexer(), type);
	auto cursor = reader.GetFirstToken();

	ParsingArguments pa(new Symbol, nullptr);
	auto program = ParseProgram(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});

	StringReader srExpect(log);
	StringReader srActual(L"\r\n" + output);

	while (true)
	{
		TEST_ASSERT(srExpect.IsEnd() == srActual.IsEnd());
		if (srExpect.IsEnd()) break;
		
		auto expect = srExpect.ReadLine();
		auto actual = srActual.ReadLine();
		TEST_ASSERT(expect == actual);
	}
}

TEST_CASE(TestParseDecl_Trivial)
{
	auto input = LR"(
)";
	auto output = LR"(
)";
	AssertProgram(input, output);
}