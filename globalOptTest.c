
int main()
{
	int a = 0;
	int b[10];
	int i = 0;
	b[i] = 1;
	b[i+1] = 1;
	for(a = b[0]; a < 10; a = b[a])
	{
		b[a] = b[a-1];
	}
}
