#include <sys/types.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "builtins.h"
#include "extern.h"

int is_builtin(command *);
void execute_builtin(command *);
void execute_cd(command *);
void execute_echo(command *);
void execute_exit();
char * get_absolute_path(char *);

int
is_builtin(command *cmd) {
	if (cmd == NULL) {
		return FALSE;
	}

	if (cmd -> args[0]) {
		if (strcasecmp(cmd -> args[0], "cd") == 0) {
			return TRUE;
		}
	
		if (strcasecmp(cmd -> args[0], "echo") == 0) {
			return TRUE;
		}

		if (strcasecmp(cmd -> args[0], "exit") == 0) {
			return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}

void
execute_builtin(command *cmd) {
	if (cmd == NULL) {
		return;
	}
	
	if (cmd -> args[0]) {
		if (strcasecmp(cmd -> args[0], "cd") == 0) {
			execute_cd(cmd);
			return;
		}
	
		if (strcasecmp(cmd -> args[0], "echo") == 0) {
			execute_echo(cmd);
			return;
		}

		if (strcasecmp(cmd -> args[0], "exit") == 0) {
			execute_exit();
			return;
		}
	}
}

void
execute_cd(command *cmd) {
	char *absolute_path;

	if (cmd == NULL) {
		exit_status = EXIT_FAILURE;
		return;
	}

	if (cmd -> argc > 2) {
		fprintf(stderr, "usage: cd [dir]\n");
		exit_status = EXIT_FAILURE;
		return;
	}

	if (cmd -> argc == 1) {
		absolute_path = get_absolute_path(NULL);
	} 
	else if (cmd -> argc == 2 && cmd -> args[1] != NULL) {
		absolute_path = get_absolute_path(cmd -> args[1]);
	}

	if (absolute_path == NULL) {
		fprintf(stderr, "cannot get absolute path\n");
		exit_status = EXIT_FAILURE;
		return;
	}

	if (chdir(absolute_path) == -1) {
		perror("chdir error");
		exit_status = EXIT_FAILURE;
		return;
	}
	
	exit_status = EXIT_SUCCESS;
}


char *
get_absolute_path(char *path) {
	uid_t uid;
	struct passwd *pwd;
	char *resolved_path = NULL;
	char *absolute_path = NULL;
	char *username = NULL;
	char *start, *end;

	if (path == NULL || (strlen(path) == 1 && path[0] == '~')) {
		uid = getuid();
		pwd = getpwuid(uid);
		if (pwd == NULL) {
			fprintf(stderr, "getpwuid error\n");
			exit_status = EXIT_FAILURE;
			return NULL;
		}
		
		return pwd -> pw_dir;
	}

	if ((absolute_path = malloc(PATH_MAX * sizeof(char))) == NULL) {
		perror("malloc");
		exit_status = EXIT_FAILURE;
		return NULL;
	}

	if ((resolved_path = malloc(PATH_MAX * sizeof(char))) == NULL) {
		perror("malloc");
		exit_status = EXIT_FAILURE;
		return NULL;
	}

	if (path[0] == '~') {
		if (path[1] == '/') {
			uid = getuid();
			pwd = getpwuid(uid);

			if (pwd == NULL) {
				fprintf(stderr, "getpwuid error\n");
				exit_status = EXIT_FAILURE;
				return NULL;
			}
			snprintf(absolute_path, PATH_MAX * sizeof(char), "%s%s", pwd -> pw_dir, path+1);
		} else {
			start = path + 1;
			end = strchr(start, '/');
			if (end == NULL) {
				if ((username = malloc((strlen(start) + 1) * sizeof(char))) == NULL) {
					perror("malloc");
					exit_status = EXIT_FAILURE;
					return NULL;
				}
				snprintf(username, (strlen(start) + 1) * sizeof(char), "%s", start);
				pwd = getpwnam(username);		
				if (pwd == NULL) {
					fprintf(stderr, "getpwnam error\n");
					exit_status = EXIT_FAILURE;
					return NULL;
				}
				snprintf(absolute_path, PATH_MAX * sizeof(char), "%s", pwd -> pw_dir);
			} else {
				if ((username = malloc((end - start + 1) * sizeof(char))) == NULL) {
					perror("malloc");
					exit_status = EXIT_FAILURE;
					return NULL;
				}
				snprintf(username, (end - start + 1) * sizeof(char), "%s", start);
				username[end - start] = '\0';
	
				pwd = getpwnam(username);		
				if (pwd == NULL) {
					fprintf(stderr, "getpwnam error\n");
					exit_status = EXIT_FAILURE;
					return NULL;
				}
				snprintf(absolute_path, PATH_MAX * sizeof(char), "%s%s", pwd -> pw_dir, end);
			}	
		}
	} else {
			snprintf(absolute_path, PATH_MAX * sizeof(char), "%s", path);
	}

	if (realpath(absolute_path, resolved_path) == NULL) {
		perror("realpath");
		exit_status = 127;
		return NULL;
	}

	if (absolute_path != NULL) {
		free(absolute_path);
	}

	if (username != NULL) {
		free(username);
	}
	
	exit_status = 0;
	return resolved_path;
}

void
execute_echo(command *cmd) {
	int i;

	if (cmd -> argc < 2) {
		fprintf(stderr, "usage: echo [word]\n");
		exit_status = EXIT_FAILURE;
		return;
	}

	for (i = 1; i < cmd -> argc; i++) {
		if (strcmp(cmd -> args[i], "$$") == 0) {
			printf("%d ", getpid());
		} else if (strcmp(cmd -> args[i], "$?") == 0) {
			if (exit_status == 0 || exit_status == 127) {
				printf("%d ", exit_status);
			} else {
				printf ("%d ", exit_status >> 8);
			}
		} else {
			printf("%s ", cmd -> args[i]);
		}

	}
	printf("\n");
	exit_status = 0;
}

void
execute_exit() {
	exit_status = 0;
	exit(0);
}
