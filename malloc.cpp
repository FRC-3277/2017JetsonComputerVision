#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

	char *string = (char *)malloc(sizeof(char) * 12);
	if ( string == NULL ) {
		fprintf(stderr, "Failed to allocate memory\n");
		return 1;
	}

	memset(string, '\0', sizeof(char) * 12);
	strncpy(string, "Hello World", sizeof(char) * 12);

	fprintf(stdout, "%s\n", string);
	free(string);

	return 0;
}
