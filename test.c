int main()
{
	int a[10];
	int n = 4;
	a[n] = 77;

	if (n)
	{
		n--;
		a[n] = 67;
	}
	else
	{
		a[n-4] = 88;
	}

	int z = 11;
}
