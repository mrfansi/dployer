#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdlib.h>

// Global database variable
extern sqlite3 *db;

// Function declarations related to database operations
void initialize_database();
void open_database(const char *db_name);
void execute_query(const char *sql);
void close_database();

#endif // DB_H