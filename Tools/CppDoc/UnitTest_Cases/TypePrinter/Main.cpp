#include "TypePrinter.h"
#include "LastArg.h"

using namespace type_printer;
using namespace last_arg;

int main()
{
	{
		// print types
		PRINT_TYPE(char);

		PRINT_TYPE(int);
		PRINT_TYPE(int const);
		PRINT_TYPE(int volatile);
		PRINT_TYPE(int const volatile);
		PRINT_TYPE(int[1]);
		PRINT_TYPE(int const[1]);
		PRINT_TYPE(int volatile[1]);
		PRINT_TYPE(int const volatile[1]);

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
	{
		struct Cat
		{
			Cat* tag = nullptr;
		};

		struct Dog
		{
			Dog* tag = nullptr;
		};

		// access member field returned from the last argument of the function call
		LastArg().tag;
		LastArg(0).tag;
		LastArg(false).tag;
		LastArg(1, true, Cat()).tag;
		LastArg(0, false, Dog()).tag;
		LastArg(Cat(), Dog(), 1).tag;
		LastArg(Dog(), Cat(), true).tag;
	}
	return 0;
}