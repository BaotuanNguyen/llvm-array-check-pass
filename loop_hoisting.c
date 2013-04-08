//void funky(void);
int main() 
{

        /*int a[100];
	int i;

	for (i = 0; i < 100; ++i) {
		a[i] = 10;
	}*/

        int top = 100;
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

        }
        //funky();

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


