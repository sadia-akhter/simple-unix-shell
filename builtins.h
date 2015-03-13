#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

int is_builtin(command *);
void execute_cd(command *);
void execute_echo(command *);
void execute_builtin(command *);
#endif
