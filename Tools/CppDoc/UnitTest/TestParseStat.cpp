#include "Util.h"
#include <Ast_Decl.h>

TEST_CASE(TestParseStat_Everything)
{
	AssertStat(
		L"{;break;continue;return;return 0; X:; X:0; default:; default:0; case 1:; case 1:0; goto X; __leave;}",
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
}

TEST_CASE(TestParseStat_SingleVar)
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
		auto funcSymbol = pa.context->children[L"F" + itow(i)][0].Obj();
		auto spa = pa.WithContextAndFunction(funcSymbol->children[L"$"][0].Obj()->children[L"$"][0].Obj()->children[L"$"][0].Obj(), funcSymbol);
		TEST_ASSERT(spa.funcSymbol->decls.Count() == 1);
		TEST_ASSERT(spa.funcSymbol->decls[0].Cast<FunctionDeclaration>()->name.name == L"F" + itow(i));
		AssertExpr(L"a", L"a", L"__int32 $L", spa);
	}
}

TEST_CASE(TestParseStat_MultiVars)
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
		auto funcSymbol = pa.context->children[L"F" + itow(i)][0].Obj();
		auto spa = pa.WithContextAndFunction(funcSymbol->children[L"$"][0].Obj()->children[L"$"][0].Obj()->children[L"$"][0].Obj(), funcSymbol);
		TEST_ASSERT(spa.funcSymbol->decls.Count() == 1);
		TEST_ASSERT(spa.funcSymbol->decls[0].Cast<FunctionDeclaration>()->name.name == L"F" + itow(i));
		AssertExpr(L"a", L"a", L"__int32 $L", spa);
		AssertExpr(L"b", L"b", L"__int32 $L", spa);
		AssertExpr(L"c", L"c", L"__int32 $L", spa);
	}
}