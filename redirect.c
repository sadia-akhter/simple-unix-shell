#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "shell.h"
#include "extern.h"
#include "redirect.h"

int input_redirect(char *);
int output_redirect(char *);
int output_append(char *);

int
io_redirection(command *cmd) {
	if (cmd == NULL) {
		exit_status = 127;
		return -1;
	}
	
	if (cmd -> input_redirect && cmd -> infile) {
		if (input_redirect(cmd -> infile) == -1) {
			exit_status = 127;
			return -1;
		}
	}

	if (cmd -> output_redirect && cmd -> outfile) {
		if (output_redirect(cmd -> outfile) == -1) {
			exit_status = 127;
			return -1;
		}
	} else if (cmd -> output_append && cmd -> outfile) {
		if (output_append(cmd -> outfile) == -1) {
			exit_status = 127;
			return -1;
		}
	}

	return 0;
}

int
input_redirect(char *filename) {
	int in;
	
	if (access(filename, F_OK) == -1 || access(filename, R_OK) == -1) {
		perror(filename);
		exit_status = 127;
		exit(127);
	}

	if ((in = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "cannot open file %s\n", filename);
		exit_status = 127;
		exit(127);	
	}

	if ((dup2(in, STDIN_FILENO)) == -1) {
		perror("dup2");
		exit_status = 127;
		exit(127);
	}

	close(in);
	return 0;
}


int
output_redirect(char *filename) {
	int out;
	struct stat buf;

	if ((out = open(filename, O_WRONLY | O_CREAT)) == -1) {
		fprintf(stderr, "cannot open file %s\n", filename);
		exit_status = 127;
		return -1;
	}

	if (lstat(filename, &buf) == -1) {
		perror("lstat");
		exit_status = 127;
		return -1;
	}

	if (S_ISREG(buf.st_mode)) {
		if (fchmod(out, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1) {
			perror("fchmod");
			exit_status = 127;
			return -1;
		}
	}

	if ((dup2(out, STDOUT_FILENO)) == -1) {
		perror("dup2");
		exit_status = 127;
		return -1;
	}
	close(out);
	return 0;
}


int
output_append(char *filename) {
	int out;
	struct stat buf;

	if ((out = open(filename, O_WRONLY | O_APPEND | O_CREAT)) == -1) {
		fprintf(stderr, "cannot open file %s\n", filename);
		exit_status = 127;
		return -1;
	}

	if (lstat(filename, &buf) == -1) {
		perror("lstat");
		exit_status = 127;
		return -1;
	}

	if (S_ISREG(buf.st_mode)) {
		if (fchmod(out, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1) {
			perror("fchmod");
			exit_status = 127;
			return -1;
		}
	}

	if ((dup2(out, STDOUT_FILENO)) == -1) {
		perror("dup2");
		exit_status = 127;
		return -1;
	}

	close(out);
	return 0;
}
