
#include <stdlib.h>
#include <stdio.h>

#include "LibArrayCheck.h"

int main(int argc, char** argv)
{
	int b[20];
	int c[10];
	int** p;
	arrayAccess(10);
	int *d = (int*)malloc(sizeof(int)*10);
	arrayAccess(9);
	int *e = (int*)malloc(sizeof(int)*5);
	p = &e;
	p[0][0] = 0x03;
        printf("end of test.c %d\n", b[0]);
	return *e + *d;
}
