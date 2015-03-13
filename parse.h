#ifndef PARSE_H
#define PARSE_H

#include "extern.h"

command ** parse_commands(char *);
command *  parse_command(char *);

void destroy_command(command *);
void destroy_commands(command **);

void print_command(command *);
void print_commands(command **);

#endif
