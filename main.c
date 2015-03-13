#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shell.h"
#include "extern.h"

void usage();
void setup_env();

int 
main(int argc, char *argv[])
{
	int opt;

	program_name = argv[0];

	while((opt = getopt(argc, argv, "xhc:")) != -1) {
		switch(opt) {
			case 'x':
				flagx = 1;
				break;
			case 'h':
				usage();
				break;
			case 'c':
				flagc = 1;
				comnd = optarg;
				break;
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 0) {
		usage();
	}

	setup_env();

	if (flagc == 0) {
		init_shell();
	} else {
		execute_shell();
	}

	exit(0);
}

void 
usage()
{
	printf("%s: -[x] [-c command]\n", program_name);
	exit(EXIT_FAILURE);
}


void
setup_env() {
	char cwd[PATH_MAX];

	if (getcwd(cwd, PATH_MAX) == NULL) {
		perror("getcwd");
		exit_status = 127;
		exit(127);
	}

	if (setenv("SHELL", cwd, TRUE) == -1) {
		perror("setenv");
		exit_status = 127;
		exit(127);
	}
}
