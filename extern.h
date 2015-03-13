#ifndef EXTERN_H
#define EXTERN_H

#define FALSE 0
#define TRUE 1

typedef struct {
	char *cmd_unparsed;
	int argc;
	char **args;
	int background;
	int job_id;
	int input_redirect;
	char *infile;
	int output_redirect;
	int output_append;
	char *outfile;
} command;


extern int exit_status;
extern char *program_name;
extern char *comnd;
extern int flagx, flagc;
extern int pipe_count;
extern int job_id;

#endif
