int main()
{
	int a[100];
	int n = 4;
	a[n] = 4;
	int* b = &n;
	int c = n;
	*b = 10000;
	a[n] = 10;
}
