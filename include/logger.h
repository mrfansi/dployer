#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>
#include <stdio.h>

// Define color codes and symbols
#define INFO "\033[1;34m"
#define SUCCESS "\033[1;32m"
#define WARNING "\033[1;33m"
#define ERROR "\033[1;31m"
#define NC "\033[0m" // No Color

#define INFO_SYMBOL "INFO"
#define SUCCESS_SYMBOL "OK"
#define WARNING_SYMBOL "WARNING"
#define ERROR_SYMBOL "ERROR"
#define INPUT_SYMBOL "\033[1;36m>\033[0m" // Cyan arrow for input symbol

// Function declaration for logging messages
void log_message(const char *color, const char *symbol, const char *message);

#endif // LOGGER_H