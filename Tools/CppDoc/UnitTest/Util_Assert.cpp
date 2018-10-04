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

void AssertType(const WString& input, const WString& log, const WString& logTsys)
{
	ParsingArguments pa(nullptr, ITsysAlloc::Create(), nullptr);
	AssertType(input, log, logTsys, pa);
}

void AssertType(const WString& input, const WString& log, const WString& logTsys, ParsingArguments& pa)
{
	CppTokenReader reader(GlobalCppLexer(), input);
	auto cursor = reader.GetFirstToken();

	auto type = ParseType(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(type, writer);
	});
	TEST_ASSERT(output == log);

	try
	{
		List<ITsys*> tsys;
		TypeToTsys(pa, type, tsys);
		TEST_ASSERT(tsys.Count() == 1);
		auto outputTsys = GenerateToStream([&](StreamWriter& writer)
		{
			Log(tsys[0], writer);
		});
		TEST_ASSERT(outputTsys == logTsys);
	}
	catch (const NotConvertableException&)
	{
		TEST_ASSERT(L"" == logTsys);
	}
}

void AssertStat(const WString& input, const WString& log)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
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