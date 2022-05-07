#include <stdio.h>
#include <unistd.h>

int main()
{
    int some = 5;
    printf("Hello world: %d\n", some);
    some += 4;
    sleep(1);
    printf("Enter a number: ");
    fflush(stdout);
    scanf("%d", &some);
    printf("You entered %d\n", some);
    return 0;
}
