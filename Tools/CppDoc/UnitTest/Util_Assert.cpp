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

void AssertStat(const WString& input, const WString& log)
{
	ParsingArguments pa(new Symbol, nullptr);
	AssertStat(input, log, pa);
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

void AssertProgram(const WString& input, const WString& log, Ptr<IIndexRecorder> recorder)
{
	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	AssertProgram(program, log);
}

void AssertProgram(Ptr<Program> program, const WString& log)
{
	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});
	AssertMultilines(output, log);
}