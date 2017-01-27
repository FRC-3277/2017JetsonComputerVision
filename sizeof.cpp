#include <stdio.h>

int main(int argc, char *argv[]) {

	/* Size of char */
	fprintf(stdout, "--- Size of char ---\n");
	fprintf(stdout, "Size of char: %lu\n\n", sizeof(char));

	/* Size of signed integers */
	fprintf(stdout, "--- Size of int ---\n");
	fprintf(stdout, "Size of int: %lu\n", sizeof(int));
	fprintf(stdout, "Size of long int: %lu\n", sizeof(long int));
	fprintf(stdout, "Size of long long int: %lu\n\n", sizeof(long long int));

	/* Size of unsigned integers */
	fprintf(stdout, "--- Size of unsigned int ---\n");
	fprintf(stdout, "Size of unsigned int: %lu\n", sizeof(unsigned int));
	fprintf(stdout, "Size of unsigned long int: %lu\n", sizeof(unsigned long int));
	fprintf(stdout, "Size of unsigned long long int: %lu\n\n", sizeof(unsigned long long int));

	/* Pointer sizes */
	fprintf(stdout, "--- Size of pointers ---\n");
	fprintf(stdout, "Size of char *: %lu\n", sizeof(char*));
	fprintf(stdout, "Size of int *: %lu\n", sizeof(int*));
	fprintf(stdout, "Size of unsigned int *: %lu\n", sizeof(unsigned int*));
	fprintf(stdout, "Size of void *: %lu\n\n", sizeof(void*));

	/* Array Sizes */
	int numbers[10];
	char string[10];
	fprintf(stdout, "--- Size of arrays ---\n");
	fprintf(stdout, "Size of int[10]: %lu\n", sizeof(numbers));
	fprintf(stdout, "Size of char[10]: %lu\n\n", sizeof(string));

	return 0;
}
