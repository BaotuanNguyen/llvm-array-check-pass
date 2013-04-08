#include <stdio.h>

//void funky(void);
int main() 
{

        // simple increment example
        /*int a[100];
	int i;

	for (i = 0; i < 100; ++i) {
		a[i] = 10;
	}*/

        // simple decrement example 
        /*int a[100];
	int i;

	for (i = 99; i >= 0; --i) {
                printf("it'ing\n");
		a[i] = 10;
	}*/


        // trivial invariant example
        /*int top = 100;
	int a[top];
	int i;
        int variant = 0;


	for (i = 0; i < 100; ++i) {
                int invariant = 0;
                variant = invariant;
		a[invariant] = 10;
		//a[invariant + 1] = 10; // this case kills me

                //for (int j = 0; j < 10; j++) {
                //        a[j] = 2;
                //}
                if (variant == invariant)
                        break;

        }*/

        // nested loop with only 1 invariant
        /*int i;
        int j;
        int a[10][10];
        for(i = 0; i < 10; ++i){
                for(j = 0; j < 10; ++j){
                        a[i][1] = 1;
                }
        }*/

        int i;
        int j;
        int a[10][10];
        for(i = 0; i < 10; ++i){
                for(j = 0; j < 10; ++j){
                        a[i][j] = 1;
                }
        }




        // uhh?
        //funky();

        printf("yo yo ma\n");
        //int oo = 0;
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


