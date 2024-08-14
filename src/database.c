#include "database.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__)
// macOS specific includes
#include <sys/syslimits.h>
#else
// Linux specific includes
#include <limits.h>
#endif

#include "logger.h"

sqlite3 *db = NULL; // Definition of the global database variable

void open_database(const char *db_name)
{
    // Get the HOME environment variable
    const char *home_dir = getenv("HOME");
    if (!home_dir)
    {
        log_message(ERROR, ERROR_SYMBOL, "Failed to get home directory.");
        exit(1);
    }

    // Construct the path to the configuration directory
    char config_dir[PATH_MAX];
    snprintf(config_dir, sizeof(config_dir), "%s/.config/dployer", home_dir);

    // Ensure the configuration directory exists
    struct stat st = {0};
    if (stat(config_dir, &st) == -1)
    {
        if (mkdir(config_dir, 0700) != 0)
        {
            perror("mkdir");
            log_message(ERROR, ERROR_SYMBOL, "Failed to create configuration directory.");
            exit(1);
        }
    }

    // Construct the full path to the database file
    char db_path[PATH_MAX];
    snprintf(db_path, sizeof(db_path), "%s/%s", config_dir, db_name);

    // Open the SQLite database
    if (sqlite3_open(db_path, &db) != SQLITE_OK)
    {
        log_message(ERROR, ERROR_SYMBOL, "Can't open database.");
        fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    else
    {
        log_message(SUCCESS, SUCCESS_SYMBOL, "Opened database successfully.");
    }

    // Check if the repositories table exists and initialize the database if needed
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
        const char *sql = "CREATE TABLE IF NOT EXISTS repositories ("
                          "id TEXT PRIMARY KEY,"
                          "git_url TEXT NOT NULL,"
                          "destination_folder TEXT NOT NULL,"
                          "branch_name TEXT NOT NULL,"
                          "docker_image_tag TEXT NOT NULL,"
                          "docker_port TEXT NOT NULL,"
                          "last_updated DATETIME DEFAULT CURRENT_TIMESTAMP"
                          ");";
        char *err_msg = NULL;
        if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK)
        {
            log_message(ERROR, ERROR_SYMBOL, "SQL error occurred.");
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            exit(1);
        }
        log_message(SUCCESS, SUCCESS_SYMBOL, "Database initialized successfully.");
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