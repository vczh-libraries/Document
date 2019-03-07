#include <stdio.h>

int main()
{
	int sum = 0;
	for (int i = 1; i <= 100; i++)
	{
		sum += i;
	}
	printf("1 + ... + 100 = %i", sum);
}