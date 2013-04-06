int main() {
	int a[100];
	int i;
        int invariant = 0;
        int variant = 0;


	for (i = 0; i < 100; ++i) {
                variant = invariant;
		a[i] = 10;
                /*for (int j = 0; j < 10; j++) {
                        a[j] = 2;
                }*/

	}	

	return 0;
}
