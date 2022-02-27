#include <stdio.h>
#include <unistd.h>

int main()
{
	int some = 5;
	printf("Hello world: %d\n", some);
	some += 4;
	printf("Updated value: %d", some);
	return 0;
}
