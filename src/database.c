#include "database.h"
#include "logger.h"
#include <sys/stat.h>

sqlite3 *db = NULL; // Definition of the global database variable

void open_database(const char *db_name)
{
    char db_path[256];
    snprintf(db_path, sizeof(db_path), "config/%s", db_name);

    // Create the config directory if it doesn't exist
    struct stat st = {0};
    if (stat("config", &st) == -1)
    {
        mkdir("config", 0700);
    }

    if (sqlite3_open(db_path, &db) != SQLITE_OK)
    {
        log_message(ERROR, ERROR_SYMBOL, "Can't open database.");
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Check if the repositories table exists
    const char *check_table_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='repositories';";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, check_table_sql, -1, &stmt, 0) != SQLITE_OK)
    {
        log_message(ERROR, ERROR_SYMBOL, "Failed to check for repositories table.");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        exit(1);
    }

    int table_exists = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        table_exists = 1;
    }
    sqlite3_finalize(stmt);

    // If the repositories table doesn't exist, initialize the database
    if (!table_exists)
    {
        initialize_database();
    }
}

void initialize_database()
{
    const char *sql = "CREATE TABLE IF NOT EXISTS repositories ("
                      "id TEXT PRIMARY KEY,"
                      "git_url TEXT NOT NULL,"
                      "destination_folder TEXT NOT NULL,"
                      "branch_name TEXT NOT NULL,"
                      "docker_image_tag TEXT NOT NULL,"
                      "docker_port TEXT NOT NULL," // Add this line
                      "last_updated DATETIME DEFAULT CURRENT_TIMESTAMP"
                      ");";

    execute_query(sql);
    log_message(SUCCESS, SUCCESS_SYMBOL, "Database initialized successfully.");
}

void close_database()
{
    if (sqlite3_close(db) != SQLITE_OK)
    {
        log_message(ERROR, ERROR_SYMBOL, "Can't close database.");
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
}

void execute_query(const char *sql)
{
    char *err_msg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK)
    {
        log_message(ERROR, ERROR_SYMBOL, "SQL error occurred.");
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(1);
    }
    log_message(SUCCESS, SUCCESS_SYMBOL, "Executed SQL query successfully.");
}