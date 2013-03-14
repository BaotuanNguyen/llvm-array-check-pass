#include <stdio.h>
#include <stdlib.h>

int a[20];
int main(int argc, char** argv)
{
	int b[10];
	int *c = malloc(sizeof(int)*10);
	printf("hello world\n");
	if(a[0] == 0)
	{
		c = c + 1;
	}
	else
	{
		c = c + 2;
	}
	a[1] = 2 + 3;
	b[9] = 10;
	c[12] = 11;
	*(c+1) = 12;
	return 0;
}

void helloFunction(int helloInt)
{
	helloInt++;
}
