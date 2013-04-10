#include <stdio.h>

//void funky(void);
int main() 
{

        // simple increment example
        int a[100];
        int ad[10][10];
	int i, j, variant;
        int invariant = 0;
		int n = 3;

        // nested loop with only 1 invariant
        for(i = 0; i < 10; ++i)
		{
                for(j = 0; j < 10; ++j)
				{
//					a[n] = 3;
                       ad[i][1] = 1;
                }
        }
}


