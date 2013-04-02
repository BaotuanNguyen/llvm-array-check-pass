
int main()
{
	int a = 0;
	int b[10];
	for(a = 1; a < 10; a++)
	{
		b[a] = b[a-1];
	}
}
