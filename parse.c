#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "parse.h"
#include "extern.h"


void init_command(command *);
int is_delim(char);
char * get_token(char *, char *);
char * get_filename(char *, char **);

command **
parse_commands(char *str) {
	command **cmd;
	char *dup, *start, *ptr, *token;	

	cmd = NULL;
	job_id = 1;

	if (str == NULL) {
		exit_status = 127;
		return NULL;
	}

	if ((dup = strdup(str)) == NULL) {
		fprintf(stderr, "strdup error\n");
		exit_status = 127;
		return NULL;
	}

	start = dup;
	pipe_count = 0;
	while (*start && (start = strchr(start, '|')) != NULL) {
		pipe_count++;
		start++;
	}

	if ((cmd = malloc((pipe_count + 1) * sizeof(command *))) == NULL) {
		fprintf(stderr, "malloc error\n");
		exit_status = 127;
		return NULL;
	}

	int i;
	for (i = 0; i < pipe_count + 1; i++) {
		cmd[i] = NULL;
	}

	start = dup;
	ptr = start;
	i = 0;
	while (*ptr) {
		if (*ptr == '|') {
			token = get_token(start, ptr);
			if (token != NULL) {
				cmd[i] = parse_command(token);
				if (cmd[i] == NULL)
					return NULL;
				i++;
			}
			ptr++;
			start = ptr;
		} else {
			ptr++;
		}
	}

	if (start < ptr) {
		token = get_token(start, ptr);
		if (token != NULL) {
			cmd[i] = parse_command(token);
			if (cmd[i] == NULL) {
				return NULL;
			}
		}
	}

	return cmd;
}

command *
parse_command(char *str) {
	char *dup, *start, *ptr, *token, *filename;
	command *cmd;

	filename = NULL;
	if (str == NULL) {
		exit_status = 127;
		return NULL;
	}

	if ((cmd = malloc(sizeof(command))) == NULL) {
		perror("malloc");
		exit_status = 127;
		return NULL;
	}

	init_command(cmd);	

	if ((dup = strdup(str)) == NULL) {
		fprintf(stderr, "strdup error\n");
		exit_status = 127;
		return NULL;
	}

	cmd -> cmd_unparsed = str;
	
	start = dup;
	ptr = start;
	while (*ptr) {
		if (is_delim(*ptr)) {
			token = get_token(start, ptr);
			if (token != NULL) {
				cmd -> job_id = job_id;
				cmd -> args[cmd -> argc] = token;
				(cmd -> argc)++;
				cmd -> args[cmd -> argc] = NULL;
			}

			if (*ptr == '&') {
				cmd -> background = TRUE;
				job_id++;
				ptr++;
				start = ptr;
			} else if (*ptr == '<') { 	// input redirection
				ptr++;
				ptr = get_filename(ptr, &filename);
				cmd -> input_redirect = TRUE;
				cmd -> infile = filename;
				start = ptr;
			} else if (*ptr == '>') {
				if (*(ptr + 1) && *(ptr + 1) == '>') { // outfile append
					ptr += 2;
					ptr = get_filename(ptr, &filename);
					cmd -> output_append = TRUE;
					cmd -> output_redirect = FALSE;
					cmd -> outfile = filename;
					start = ptr;
				} else {		// output redirection
					ptr++;
					ptr = get_filename(ptr, &filename);
					cmd -> output_redirect = TRUE;
					cmd -> output_append = FALSE;
					cmd -> outfile = filename;
					start = ptr;
				}
			} else {
				ptr++;
				start = ptr;
			}
		} else {
			ptr++;
		}
	}

	if (start < ptr) {
		token = get_token(start, ptr);
		cmd -> args[cmd -> argc] = token;
		(cmd -> argc)++;
		cmd -> args[cmd -> argc] = NULL;
	}

	return cmd;

}

void 
init_command(command *cmd) {
	cmd -> cmd_unparsed = NULL;
	cmd -> background = FALSE;
	cmd -> input_redirect = FALSE;
	cmd -> infile = NULL;
	cmd -> output_redirect = FALSE;
	cmd -> output_append = FALSE;
	cmd -> outfile = NULL;
	
	if ((cmd -> args = malloc(sysconf(_SC_ARG_MAX) * sizeof(char *))) == NULL) {
		perror("malloc error");
		exit(EXIT_FAILURE);
	}
	
	cmd -> args[0] = NULL; 
	cmd -> argc = 0;
}

void
destroy_commands(command **cmds) {
	int i;
	for (i = 0; i < pipe_count + 1; i++) {
		if (cmds[i] != NULL) {
			destroy_command(cmds[i]);
		}
	}
}

void 
destroy_command(command *cmd) {
	int i;

	cmd -> cmd_unparsed = NULL; 
	
	if (cmd -> infile != NULL) {
		free(cmd -> infile);
	}

	if (cmd -> outfile != NULL) {
		free(cmd -> outfile);
	}
	
	for (i = 0; i < cmd -> argc; i++) {
		if (cmd -> args[i] != NULL) {
			free(cmd -> args[i]);
		}
	}

	if (cmd -> args != NULL) {
		free(cmd -> args);
	}
}

char *
get_filename(char *ptr, char **filename) {
	char *start;

	if (ptr == NULL) {
		return NULL;
	}

	while ((*ptr) && isblank(*ptr)) { // ignore leading blank chars
		ptr++;
	}
	start = ptr;

	while ((*ptr) && !is_delim(*ptr)) {
		ptr++;
	}
	
	if ((*filename = malloc((ptr - start + 1) * sizeof(char))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	strncpy(*filename, start, ptr - start);
	(*filename)[ptr - start] = '\0';
	return ptr;
}

char *
get_token(char *start, char *end) {
	char *token;
	if (start >= end) {
		return NULL;
	}

	if ((token = malloc((end - start + 1) * sizeof(char))) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	snprintf(token, (end - start + 1) * sizeof(char), "%s", start);
	token[end - start] = '\0';
	return token;
}

int
is_delim(char ch) {
	switch(ch) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case '<':
		case '>':
		case '&':
			return TRUE;
			break;
		default:
			return FALSE;
	}
}

void
print_commands(command **cmds) {
	if (flagx == 0) {
		return;
	}

	if (cmds == NULL) {
		return;
	}
	
	int i;
	for (i = 0; i < pipe_count + 1; i++) {
		if (cmds[i] != NULL) {
			fprintf(stderr, "+ %s\n", cmds[i] -> cmd_unparsed);
		}
	}

}

void
print_command(command *cmd) {
	int i;

	if (cmd -> cmd_unparsed != NULL) { 
		printf("cmd_unparsed: %s\n", cmd -> cmd_unparsed);
	}

	printf("background: %d\n", cmd->background);
	printf("job_id: %d\n", cmd->job_id);
	printf("input redirect: %d\n", cmd->input_redirect);

	if (cmd -> infile != NULL) {
		printf("infile: %s\n", cmd -> infile);
	}

	printf("output redirect: %d\n", cmd -> output_redirect);
	printf("output append: %d\n", cmd -> output_append);
	
	if (cmd -> outfile != NULL) {
		printf("outfile: %s\n", cmd -> outfile);
	}
	
	for (i = 0; i < cmd -> argc; i++) {
		if (cmd -> args[i] != NULL) {
			printf("args[%d]: %s\n", i, cmd -> args[i]);
		}
	}
}

