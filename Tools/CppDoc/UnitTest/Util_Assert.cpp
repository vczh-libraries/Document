#include "Util.h"

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

void AssertLog(const WString& output, const wchar_t* log)
{
	if (output != log)
	{
		TEST_PRINT(L"Expect: " + WString(log));
		TEST_PRINT(L"Actual: " + WString(output));
		throw unittest::UnitTestAssertError(L"Log mismatched");
	}
}

void PrintMultiple(SortedList<WString>& values, const wchar_t* label)
{
	if (values.Count() == 0)
	{
		TEST_PRINT(label + WString(L"<EMPTY>"));
	}
	else if (values.Count() == 1)
	{
		TEST_PRINT(label + values[0]);
	}
	else
	{
		TEST_PRINT(label);
		for (vint i = 0; i < values.Count(); i++)
		{
			TEST_PRINT(L"    " + values[i]);
		}
	}
}

void AssertTsys(SortedList<WString>& actuals, const wchar_t** logTsys, vint count)
{
	SortedList<WString> expects;
	for (vint i = 0; i < count; i++)
	{
		expects.Add(logTsys[i]);
	}

	if ((CompareEnumerable(expects, actuals) != 0))
	{
		PrintMultiple(expects, L"Expect: ");
		PrintMultiple(actuals, L"Actual: ");
		throw unittest::UnitTestAssertError(L"Type mismatched");
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
	AssertLog(output, (log ? log : input));

	TypeTsysList tsys;
	if (count > 0)
	{
		TypeToTsysNoVta(pa, type, tsys);
	}
	else
	{
		try
		{
			TypeToTsysNoVta(pa, type, tsys);
		}
		catch (const TypeCheckerException&) {}
	}

	SortedList<WString> actuals;
	for (vint i = 0; i < tsys.Count(); i++)
	{
		actuals.Add(GenerateToStream([&](StreamWriter& writer)
		{
			Log(tsys[i], writer);
		}));
	}
	AssertTsys(actuals, logTsys, count);
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
	AssertLog(output, (log ? log : input));

	ExprTsysList tsys;
	if (count > 0)
	{
		ExprToTsysNoVta(pa, expr, tsys);
	}
	else
	{
		try
		{
			ExprToTsysNoVta(pa, expr, tsys);
		}
		catch (const IllegalExprException&) {}
		catch (const TypeCheckerException&) {}
	}

	SortedList<WString> actuals;
	for (vint i = 0; i < tsys.Count(); i++)
	{
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
	AssertTsys(actuals, logTsys, count);
}

/***********************************************************************
AssertStat
***********************************************************************/

void AssertStat(const wchar_t* input, const wchar_t* log)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
	AssertStat(pa, input, log);
}

void AssertStat(ParsingArguments& pa, const wchar_t* input, const wchar_t* log)
{
	TEST_CASE(L"[STAT] " + WString(input))
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
	});
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
	TEST_CASE(L"[PROG] <hidden>")
	{
		auto output = GenerateToStream([&](StreamWriter& writer)
		{
			Log(program, writer);
		});
		AssertMultilines(output, log);
	});
}