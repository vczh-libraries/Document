#include "Util.h"

/***********************************************************************
AssertMultilines
***********************************************************************/

void RefineInput(wchar_t* input)
{
	auto reading = input;
	while (true)
	{
		reading = wcsstr(reading, L" _ ");
		if (!reading) break;
		reading[1] = L',';
		reading += 3;
	}
}

/***********************************************************************
AssertMultilines
***********************************************************************/

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

/***********************************************************************
AssertType
***********************************************************************/

void AssertTypeInternal(const wchar_t* input, const wchar_t* log, const wchar_t** logTsys, vint count, ParsingArguments& pa)
{
	TOKEN_READER(input);
	auto cursor = reader.GetFirstToken();

	auto type = ParseType(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(type, writer);
	});
	TEST_ASSERT(output == log);

	TypeTsysList tsys;
	try
	{
		TypeToTsys(pa, type, tsys, nullptr);
	}
	catch (const NotConvertableException&)
	{
		TEST_ASSERT(count == 0);
		return;
	}

	TEST_ASSERT(tsys.Count() == count);

	SortedList<WString> expects, actuals;
	for (vint i = 0; i < count; i++)
	{
		expects.Add(logTsys[i]);
		actuals.Add(GenerateToStream([&](StreamWriter& writer)
		{
			Log(tsys[i], writer);
		}));
	}

	TEST_ASSERT(CompareEnumerable(expects, actuals) == 0);
}

/***********************************************************************
AssertExpr
***********************************************************************/

void AssertExprInternal(const wchar_t* input, const wchar_t* log, const wchar_t** logTsys, vint count, ParsingArguments& pa)
{
	TOKEN_READER(input);
	auto cursor = reader.GetFirstToken();

	auto expr = ParseExpr(pa, pea_Full(), cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(expr, writer);
	});
	TEST_ASSERT(output == log);

	ExprTsysList tsys;
	try
	{
		ExprToTsys(pa, expr, tsys);
	}
	catch (const IllegalExprException&)
	{
		TEST_ASSERT(count == 0);
		return;
	}

	TEST_ASSERT(tsys.Count() == count);

	SortedList<WString> expects, actuals;
	for (vint i = 0; i < count; i++)
	{
		expects.Add(logTsys[i]);
		actuals.Add(GenerateToStream([&](StreamWriter& writer)
		{
			Log(tsys[i].tsys, writer);
			switch (tsys[i].type)
			{
			case ExprTsysType::LValue:
				writer.WriteString(L" $L");
				break;
			case ExprTsysType::PRValue:
				writer.WriteString(L" $PR");
				break;
			case ExprTsysType::XValue:
				writer.WriteString(L" $X");
				break;
			}
		}));
	}

	TEST_ASSERT(CompareEnumerable(expects, actuals) == 0);
}

/***********************************************************************
AssertStat
***********************************************************************/

void AssertStat(const wchar_t* input, const wchar_t* log)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
	AssertStat(pa, input, log);
}

void AssertStat(ParsingArguments& pa, const wchar_t* input, const wchar_t* log)
{
	TOKEN_READER(input);
	auto cursor = reader.GetFirstToken();

	auto stat = ParseStat(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(stat, writer, 0);
	});

	AssertMultilines(output, log);
}

/***********************************************************************
AssertProgram
***********************************************************************/

void AssertProgram(const wchar_t* input, const wchar_t* log, Ptr<IIndexRecorder> recorder)
{
	COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
	AssertProgram(program, log);
}

void AssertProgram(Ptr<Program> program, const wchar_t* log)
{
	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});
	AssertMultilines(output, log);
}