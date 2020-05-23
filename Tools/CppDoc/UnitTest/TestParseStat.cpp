#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Everything")
	{
		AssertStat(
			L"{;break;continue;return;return 0; X:; X:0; default:; default:0; case 1:; case 1:0; goto X; __leave; static_assert(sizeof(int) == 4);}",
			LR"(
{
	;
	break;
	continue;
	return;
	return 0;
	X: ;
	X: 0;
	default: ;
	default: 0;
	case 1: ;
	case 1: 0;
	goto X;
	__leave;
	static_assert((sizeof(int) == 4));
}
)");

		AssertStat(
			L"while(int x=1){do{;}while(0);}",
			LR"(
while (x: int = 1)
{
	do
	{
		;
	}
	while (0);
}
)");

		AssertStat(
			L"for(int x:0) for(;;) for(0;1;2) for(int i=0,j=0;1;2) ;",
			LR"(
foreach (x: int : 0)
	for (; ; )
		for (0; 1; 2)
			for (i: int = 0, j: int = 0; 1; 2)
				;
)");

		AssertStat(
			L"if (int i=0) if (0) 1; else if (int i=0,j=0;1) 2; else 3;",
			LR"(
if (i: int = 0)
	if (0)
		1;
	else if (i: int = 0, j: int = 0; 1)
		2;
	else
		3;
)");

		AssertStat(
			L"switch(0){case 1:1; break; case 2:2; default: switch(int i=0);}",
			LR"(
switch (0)
{
	case 1: 1;
	break;
	case 2: 2;
	default: switch (i: int = 0)
		;
}
)");

		AssertStat(
			L"try try{1;2;3;}catch(...); catch(int) try; catch(int x);",
			LR"(
try
	try
	{
		1;
		2;
		3;
	}
	catch (...)
		;
catch (int)
	try
		;
	catch (x: int)
		;
)");

		AssertStat(
			L"__try; __except(0) __try; __finally;",
			LR"(
__try
	;
__except (0)
	__try
		;
	__finally
		;
)");
	});

	auto GetStatementSymbol = [](const ParsingArguments& pa, const WString& functionName)
	{
		auto functionSymbol = pa.scopeSymbol->TryGetChildren_NFb(functionName)->Get(0).childSymbol;
		auto functionBodySymbol = functionSymbol->GetImplSymbols_F()[0];
		auto bodyStat = functionBodySymbol->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
		auto controlFlowStat = bodyStat->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
		auto controlFlowBlock = controlFlowStat->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
		return controlFlowBlock.Obj();
	};

	TEST_CATEGORY(L"Single variables")
	{
		auto input = LR"(
void F1()
{
	{
		int a = 0;
		{
		}
	}
}

void F2()
{
	while (int a = 0)
	{
	}
}

void F3()
{
	int v[10];
	for (int a : v)
	{
	}
}

void F4()
{
	for (int a = 0; a < 10; a++)
	{
	}
}

void F5()
{
	if (int a = 0)
	{
	}
}

void F6()
{
	if (int a = 0; true)
	{
	}
}

void F7()
{
	switch (int a = 0)
	{
	}
}

void F8()
{
	try;
	catch (int a)
	{
	}
}
)";
		COMPILE_PROGRAM(program, pa, input);
		for (vint i = 1; i <= 8; i++)
		{
			auto spa = pa.WithScope(GetStatementSymbol(pa, L"F" + itow(i)));
			TEST_CASE_ASSERT(spa.functionBodySymbol->GetImplDecl_NFb<FunctionDeclaration>()->name.name == L"F" + itow(i));
			AssertExpr(spa, L"a", L"a", L"__int32 $L");
		}
	});

	TEST_CATEGORY(L"Multiple variables")
	{
		auto input = LR"(
void F1()
{
	{
		int a = 0, b;
		int c;
		{
		}
	}
}

void F2()
{
	for (int a = 0, b = 0, c = 0; a < 0; a++, b++, c++)
	{
	}
}

void F3()
{
	if (int a, b; int c = 0)
	{
	}
}

void F4()
{
	if (int a, b, c; true)
	{
	}
}
)";
		COMPILE_PROGRAM(program, pa, input);
		for (vint i = 1; i <= 4; i++)
		{
			auto spa = pa.WithScope(GetStatementSymbol(pa, L"F" + itow(i)));
			TEST_CASE_ASSERT(spa.functionBodySymbol->GetImplDecl_NFb<FunctionDeclaration>()->name.name == L"F" + itow(i));
			AssertExpr(spa, L"a", L"a", L"__int32 $L");
			AssertExpr(spa, L"b", L"b", L"__int32 $L");
			AssertExpr(spa, L"c", L"c", L"__int32 $L");
		}
	});

	TEST_CATEGORY(L"Range-based for")
	{
		auto input = LR"(
void F1()
{
	int v[10];
	for (auto& a : v)
	{
	}
}

namespace std2
{
	struct Vector
	{
		int* begin();
		int* end();
	};
}

void F2()
{
	for (const auto& a : std2::Vector())
	{
	}
}

namespace std3
{
	struct Vector
	{
		struct Iterator
		{
			int& operator*();
		};
	};

	Vector::Iterator begin(const Vector&);
	Vector::Iterator end(const Vector&);
}

void F3()
{
	for (decltype(auto) a : std3::Vector())
	{
	}
}
)";
		COMPILE_PROGRAM(program, pa, input);
		const wchar_t* expectedTypes[] = { L"__int32 & $L", L"__int32 const & $L", L"__int32 & $L" };
		for (vint i = 1; i <= 3; i++)
		{
			auto spa = pa.WithScope(GetStatementSymbol(pa, L"F" + itow(i)));
			TEST_CASE_ASSERT(spa.functionBodySymbol->GetImplDecl_NFb<FunctionDeclaration>()->name.name == L"F" + itow(i));
			AssertExpr(spa, L"a", L"a", expectedTypes[i - 1]);
		}
	});
}