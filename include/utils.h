#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function declarations for utility functions
void get_input(const char *prompt, char *input, size_t size);
void execute_command(const char *command);
void check_requirements();

#endif // UTILS_H