#include <Ast_Stat.h>
#include "Util.h"

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
		L"while(1){do{;}while(0);}",
		LR"(
while (1)
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
	for (
		;
		;
		)
		for (
			0;
			1;
			2)
			for (
				i: int = 0;
				j: int = 0;
				1;
				2)
				;
)");

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
)");

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
catch
	int;
	try
		;
	catch
		int x;
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