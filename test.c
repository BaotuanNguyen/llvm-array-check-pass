int d[3][4];
int p[10];

int main(int argc, char** argv)
{
	int i = 0;
	int n = 10;
	int a[n][7];
	int b[10][n][n+3];
	a[9][4] = 4;
	b[1][2][3] = 10;
	d[1][3] = 11;
//	printf("argc %d\n", argc);
	for(i = 0; i < argc; i++)
	{
		p[i] = 0;
		p[i] = p[i+1] + i;
	}
}


void test(int n)
{
	int dyn[n];
	dyn[n-1] = 44;
}
