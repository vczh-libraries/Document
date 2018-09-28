#include "Util.h"

void AssertMultilines(const WString& output, const WString& log)
{
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

void AssertType(const WString& input, const WString& log)
{
	ParsingArguments pa;
	AssertType(input, log, pa);
}

void AssertType(const WString& input, const WString& log, ParsingArguments& pa)
{
	CppTokenReader reader(GlobalCppLexer(), input);
	auto cursor = reader.GetFirstToken();

	List<Ptr<Declarator>> declarators;
	ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
	TEST_ASSERT(!cursor);
	TEST_ASSERT(declarators.Count() == 1);
	TEST_ASSERT(!declarators[0]->name);
	TEST_ASSERT(declarators[0]->initializer == nullptr);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(declarators[0]->type, writer);
	});
	TEST_ASSERT(output == log);
}

void AssertStat(const WString& input, const WString& log, ParsingArguments& pa)
{
	CppTokenReader reader(GlobalCppLexer(), input);
	auto cursor = reader.GetFirstToken();

	auto stat = ParseStat(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(stat, writer, 0);
	});

	AssertMultilines(output, log);
}

void AssertProgram(const WString& input, const WString& log)
{
	COMPILE_PROGRAM(program, pa, input);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});

	AssertMultilines(output, log);
}