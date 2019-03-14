#include <stdio.h>
#include <string.h>
#include "Calculator.h"

using namespace calculator;

#define MAX_BUFFER 1024

// See if the parameter type is correctly rendered, by clicking the JustForFun call in the main function.
void JustForFun() {}
void JustForFun(const int*(&(__fastcall * const&)(const volatile Expr &&, int Expr::Ptr::*))[10]) {}

int main()
{
	char input[MAX_BUFFER];
	while (true)
	{
		scanf_s("%1023s", input, (unsigned)MAX_BUFFER);
		if (strcmp(input, "exit") == 0)
		{
			break;
		}

		auto expr = Parse(input);
		Print(expr);
		printf(" = %g\n", Evaluate(expr));
	}
	JustForFun(nullptr);
	return 0;
}