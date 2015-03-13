#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "parse.h"
#include "extern.h"
#include "builtins.h"
#include "redirect.h"

void signal_handler(int);
void signal_handler2(int);
char * get_command();
char * trim_trailing_newline(char *);

void execute_commands(command **);
void execute_command(command *);
void execute_as_command(command *);

void
execute_shell() {
	command **cmds = NULL;
	cmds = parse_commands(comnd);
	print_commands(cmds);
	execute_commands(cmds);
	destroy_commands(cmds);
}

void
init_shell() {	
	char *line = NULL;
	command **cmds = NULL;

	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		fprintf(stderr, "cannot set signal handler\n");
		exit(127);
	}

	do {
		if ((line = get_command()) == NULL) {
			continue;
		}
		if ((cmds = parse_commands(line)) == NULL) {
			continue;
		}
		print_commands(cmds);
		execute_commands(cmds);
		destroy_commands(cmds);
	} while (TRUE);
	exit(0);
}



void
execute_commands(command **cmds) {
	if (pipe_count == 0) {
		execute_command(cmds[0]);
		return;
	}

	int pipes[pipe_count][2];
	int i, j;
	pid_t pid, wpid;

	for (i = 0; i < pipe_count; i++) {
		if (pipe(pipes[i]) == -1) {
			perror("pipes");
			exit_status = 127;
			return;
		}
	}

	for (i = 0; i < pipe_count; i++) {
		if (cmds[i] != NULL) {
			pid = fork();
			if (pid == -1) {	
				perror("fork");
				exit_status = 127;
				return;
			} else if (pid == 0) {
				close(pipes[i][0]);
				if (pipes[i][1] != STDOUT_FILENO) {
						dup2(pipes[i][1], STDOUT_FILENO);
						close(pipes[i][1]);
				}

				if (i != 0) {
					close(pipes[i - 1][1]);
					if (pipes[i-1][0] != STDIN_FILENO) {
						dup2(pipes[i-1][0], STDIN_FILENO);
						close(pipes[i-1][0]);
					}
				} 
				
				if (i == 0) {
					for (j = 1; j < pipe_count; j++) {
						close(pipes[j][0]);
						close(pipes[j][1]);
					}
				} else {
					for (j = 0; j < pipe_count; j++) {
						if (j != i && j != i-1) {
							close(pipes[j][0]);
							close(pipes[j][1]);
						}
					}
				}
		
				if (io_redirection(cmds[i]) == -1) {
					exit_status = 127;
					exit(127);
				}

				if (execvp(cmds[i] -> args[0], cmds[i] -> args) < 0) {
					fprintf(stderr, "%s: command not found\n", cmds[i] -> args[0]);
					exit_status = 127;
					exit(127);
				}

			}
		}
	}

	pid = fork();
	if (pid == -1) {	
		perror("fork");
		exit_status = 127;
		return;
	} else if (pid == 0) {
		close(pipes[i-1][1]);
		if (pipes[i-1][0] != STDIN_FILENO) {
			dup2(pipes[i-1][0], STDIN_FILENO);
			close(pipes[i-1][0]);
		}

		for (j = 0; j < pipe_count - 1; j++) {
			close(pipes[j][0]);
			close(pipes[j][1]);
		}

		if (io_redirection(cmds[i]) == -1) {
			exit_status = 127;
			exit(127);
		}
		
		if (execvp(cmds[i] -> args[0], cmds[i] -> args) < 0) {
			fprintf(stderr, "%s: command not found\n", cmds[i] -> args[0]);
			exit_status = 127;
			exit(127);
		}

	} 
	
	for (j = 0; j < pipe_count; j++) {
		close(pipes[j][0]);
		close(pipes[j][1]);
	}

	while ((wpid = wait(&exit_status) > 0));
}

void 
execute_command(command *cmd) {
	if (is_builtin(cmd)) {
		execute_builtin(cmd);
	} else {
		execute_as_command(cmd);
	}
}

void 
execute_as_command(command *cmd) {
	pid_t pid;

	if (cmd == NULL) {
		return;
	}

	if (cmd -> background == TRUE) {
	//	fprintf(stdout, "[%d] \t %d\n", cmd->job_id, getpid());
	
		if (signal(SIGCHLD, signal_handler2) == SIG_ERR) {
			fprintf(stderr, "cannot set signal handler\n");
			exit(127);
		}
	} else {
		if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) {
			fprintf(stderr, "cannot set signal handler to default\n");
			exit(127);
		}
	}

	if ((pid = fork()) == -1) {
		perror("fork");
		exit_status = 127;
		return;
	} else if (pid == 0) {
		if (io_redirection(cmd) == -1) {
			exit_status = 127;
			exit(127);
		}
		
		if (cmd -> background == TRUE) {
			setsid();
		}

		if (execvp(cmd -> args[0], cmd -> args) < 0) {
			fprintf(stderr, "%s: command not found\n", cmd -> args[0]);
			exit_status = 127;
			exit(127);
		}
	} else {
		if (cmd -> background == FALSE) {
			waitpid(pid, &exit_status, 0);
			fflush(stdout);
		}
	}
}


char *
get_command() {
	char *line = NULL;
	size_t size;

	printf("sish$ ");
	if (getline(&line, &size, stdin) == -1) {
		fprintf(stderr, "%s: could not read from stdin\n", program_name);
		exit_status = 127;
		exit(127);
	}

	line = trim_trailing_newline(line);

	return line;
}

char *
trim_trailing_newline(char *str) {
    char *endp;
    endp = str + strlen(str)-1;
    while(endp > str && (*endp == '\r' || *endp == '\n')) {
        *endp = '\0';
        endp--;
    }

    return str;
}


void 
signal_handler(int sig) {
	if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
		fprintf(stderr, "cannot set signal handler to SIG_IGN\n");
	}
}


void 
signal_handler2(int sig) {
	if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
		fprintf(stderr, "cannot set signal handler\n");
	}
}

