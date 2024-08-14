#include "logger.h"

void log_message(const char *color, const char *symbol, const char *message)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Use a consistent label "[LOG]" for all log levels and ensure alignment
    printf("%s[LOG] %s [%04d-%02d-%02d %02d:%02d:%02d] %s%s\n",
           color, symbol,
           tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
           tm.tm_hour, tm.tm_min, tm.tm_sec,
           message, NC);
}