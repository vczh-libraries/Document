#include "TypePrinter.h"

using namespace type_printer;

int main()
{
	{
		// print types
		PRINT_TYPE(char);
		PRINT_TYPE(int);

		PRINT_TYPE(char*);
		PRINT_TYPE(const char*const);
		PRINT_TYPE(int&);
		PRINT_TYPE(volatile int&&);

		PRINT_TYPE(int[10]);
		PRINT_TYPE(const int[1][2][3]);
		PRINT_TYPE(volatile int(&)[4][5]);
		PRINT_TYPE(const char(*const)[4][5]);

		PRINT_TYPE(int());
		PRINT_TYPE(int(*)(bool, char));

		PRINT_TYPE(int(&(*)(const bool(*)(), volatile const char(&)[1]))[10]);
	}
	return 0;
}