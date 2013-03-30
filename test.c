
int main(int argc, char** argv)
{
	int a[10][30];
	int i, j;
	for(i = 0; i < 10; i++)
	{
		for(j = 0; j < 30; j++)
		{
			a[i][j] = a[0][j] + a[0][j+1];
		}
	}
}

