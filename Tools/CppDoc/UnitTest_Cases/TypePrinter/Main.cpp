#include <stdio.h>
#include <string.h>
#include "TypePrinter.h"

using namespace type_printer;

int main()
{
	{
		// print types
		PrintType<char>();
		PrintType<int>();

		PrintType<char*>();
		PrintType<const char*>();
		PrintType<int&>();
		PrintType<volatile int&&>();

		PrintType<int[10]>();
		PrintType<const int[1][2][3]>();
		PrintType<volatile int(&)[4][5]>();
		PrintType<const char(*const)[4][5]>();

		PrintType<int()>();
		PrintType<int(*)(bool, char)>();

		PrintType<int(&(*)(const bool(*)(), volatile char(&)[1]))[10]>();
	}
	return 0;
}