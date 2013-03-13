#include <stdio.h>
#include <stdlib.h>

int a[20];
int main(int argc, char** argv)
{
	int b[10];
	int *c = malloc(sizeof(int)*10);
	printf("hello world\n");
	a[1] = 2 + 3;
	b[9] = 10;
	c[0] = 11;
	*(c+1) = 12;
	return 0;
}

void helloFunction(int helloInt)
{
	helloInt++;
}
