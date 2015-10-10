/*
 * OS Assignment #2
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>



int main(int argc, char **argv) {
	if (argc <= 1) {
		fprintf (stderr, "usage: %s input-file\n", argv[0]);
		return -1;
	}

	return 0;
}
