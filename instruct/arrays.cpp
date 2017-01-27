#include <stdio.h>

int main(int argc, char *argv[]) {

	/* Call taken per-day */
	int calls[7];
	calls[0] = 3064;
	calls[1] = 2864;
	calls[2] = 2908;
	calls[3] = 2759;
	calls[4] = 2704;
	calls[5] = 2459;
	calls[6] = 2882;

	fprintf(stdout, "Calls taken Monday (Day 0): %d\n", calls[0]);
	fprintf(stdout, "Calls taken Tuesday (Day 1): %d\n", calls[1]);
	fprintf(stdout, "Calls taken Wednesday (Day 2): %d\n", calls[2]);
	fprintf(stdout, "Calls taken Thursday (Day 3): %d\n", calls[3]);
	fprintf(stdout, "Calls taken Friday (Day 4): %d\n", calls[4]);
	fprintf(stdout, "Calls taken Saturday (Day 5): %d\n", calls[5]);
	fprintf(stdout, "Calls taken Sunday (Day 6): %d\n", calls[6]);


	/* Packages sent per-day */
	int packages[7] = {15360,16285,16865,18542,15209,15209,16167};
	for ( int i = 0; i < 7; i++ ) {
		fprintf(stdout, "Day: %d, Packages: %d\n", i, packages[i]);
	}

	return 0;
}
