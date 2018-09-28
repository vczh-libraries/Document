#include <Ast_Stat.h>
#include "Util.h"

TEST_CASE(TestParseStat_Everything)
{
	ParsingArguments pa;

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
)", pa);

	AssertStat(
		L"while(true){do{;}while(false);}",
		LR"(
while (true)
{
	do
	{
		;
	}
	while (false);
}
)", pa);

	AssertStat(
		L"for(int x:0) for(;;) for(0;1;2) for(int i=0,j=0;1;2) ;",
		LR"(
for
	int x;
	0
	for
		0;
		1;
		2
		for
			int i = 0;
			int j = 0;
			1;
			2
			;
)", pa);

	AssertStat(
		L"if (int i=0) if (0) 1; else if (1) 2; else 3;",
		LR"(
if
	int i = 0;
	if (0)
		1;
	else if (1)
		2;
	else
		3;
)", pa);

	AssertStat(
		L"switch(0){case 1:1; break; case 2:2; default:0;}",
		LR"(
switch (0)
{
	case 1: 1;
	break;
	case 2: 2;
	default: 0;
}
)", pa);

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
catch
	int;
	try
		;
	catch
		int x;
		;
)", pa);

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
)", pa);
}