#include <stdio.h>
#include <string.h>
#include "Calculator.h"

using namespace calculator;

#define MAX_BUFFER 1024

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
		printf(" = %f\n", Evaluate(expr));
	}
	return 0;
}