#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {

	/* Create char array of size 12 */
	char string[12];

	/* Copy "Hello World" into array and print to console */
	strncpy(string, "Hello World", sizeof(string));
	fprintf(stdout, "%s\n", string);

	/* Create a pointer that points to the string array and print */
	char * copy = string;
	fprintf(stdout, "Copy of string: %s\n", copy);

	/* Why are the sizes different? */
	fprintf(stdout, "Copy size: %lu, String size: %lu\n", sizeof(copy), sizeof(string));

	/* Print pointer values */
	fprintf(stdout, "String ptr: %p, Copy ptr: %p\n", string, copy);

	/* Move pointer ahead 6 bytes */
	copy = copy + 6;
	fprintf(stdout, "String ptr: %p, Copy ptr: %p\n", string, copy);
	fprintf(stdout, "Move pointer: %s\n", copy);

	return 0;
}
