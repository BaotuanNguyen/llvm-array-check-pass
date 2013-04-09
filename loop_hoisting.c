#include <stdio.h>

//void funky(void);
int main() 
{

        // simple increment example
        int a[100];
        int ad[10][10];
	int i, j, variant;
        int invariant = 0;

	/*for (i = 0; i < 100; ++i) {
		a[i] = 10;
	}*/

        // simple decrement example 
	/*for (i = 99; i >= 0; --i) {
                printf("it'ing\n");
		a[i] = 10;
	}*/


        // trivial invariant example
	/*for (i = 0; i < 100; ++i) {
                variant = invariant;
		a[invariant] = 10;
		//a[invariant + 1] = 10; // this case kills me

                if (variant == invariant)
                        break;
        }*/

        // nested loop with only 1 invariant
        /*for(i = 0; i < 10; ++i){
                for(j = 0; j < 10; ++j){
                        ad[i][1] = 1;
                }
        }*/

        // nested loop with two increments
        for(i = 0; i < 10; ++i){
                for(j = 0; j < 10; ++j){
                        ad[i][j] = 1;
                }
        }



        // uhh?
        //funky();

        printf("yo yo ma\n");
	return 0;
}


/*void funky(void)
{
        int ok;
        int arr[10];
        for(ok = 0; ok < 10; ++ok) {
               arr[ok] = 1; 
        }
}*/


