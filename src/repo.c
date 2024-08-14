#include "repo.h"
#include "logger.h"
#include "database.h"
#include "utils.h"
#include "docker.h"
#include <json-c/json.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_PATH_LEN 4096

int is_php_version_82_or_higher(const char *version)
{
  // Remove any leading spaces or special characters (like ^, ~)
  while (*version && !isdigit(*version))
    version++;

  // Check if the version is 8.2 or higher
  if (strncmp(version, "8.2", 3) >= 0)
  {
    return 1;
  }

  return 0;
}

int check_laravel_and_php_versions(const char *repo_path)
{
  char composer_json_path[PATH_MAX];
  snprintf(composer_json_path, sizeof(composer_json_path), "%s/composer.json", repo_path);

  FILE *file = fopen(composer_json_path, "r");
  if (!file)
  {
    log_message(WARNING, WARNING_SYMBOL, "Could not open composer.json file.");
    return 0;
  }

  // Read the composer.json file
  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *data = (char *)malloc(length + 1);
  if (data)
  {
    fread(data, 1, length, file);
    data[length] = '\0';
  }
  fclose(file);

  // Parse the JSON
  struct json_object *parsed_json = json_tokener_parse(data);
  struct json_object *require;
  struct json_object *php_version;

  int use_php82 = 0;

  if (json_object_object_get_ex(parsed_json, "require", &require))
  {
    if (json_object_object_get_ex(require, "php", &php_version))
    {
      const char *php_version_str = json_object_get_string(php_version);
      char version_message[256];
      snprintf(version_message, sizeof(version_message), "Minimum PHP version required: %s", php_version_str);
      log_message(INFO, INFO_SYMBOL, version_message);

      // Split version string by "|"
      char *version_token = strtok((char *)php_version_str, "|");
      while (version_token != NULL)
      {
        if (is_php_version_82_or_higher(version_token))
        {
          use_php82 = 1;
          break;
        }
        version_token = strtok(NULL, "|");
      }
    }
  }

  json_object_put(parsed_json);
  free(data);

  return use_php82;
}

const char *check_repo_framework(const char *repo_path)
{
  struct stat st;

  // Check for Laravel
  char laravel_check[256];
  snprintf(laravel_check, sizeof(laravel_check), "%s/artisan", repo_path);
  if (stat(laravel_check, &st) == 0)
  {
    return "laravel";
  }

  // Check for static PHP (index.php or other common static PHP indicators)
  char static_php_check[256];
  snprintf(static_php_check, sizeof(static_php_check), "%s/index.php", repo_path);
  if (stat(static_php_check, &st) == 0)
  {
    return "static-php";
  }

  // If neither Laravel nor static PHP
  return "unknown";
}

void ensure_repositories_folder_exists()
{
  struct stat st = {0};
  if (stat("repositories", &st) == -1)
  {
    if (mkdir("repositories", 0700) != 0)
    {
      perror("mkdir");
      log_message(ERROR, ERROR_SYMBOL, "Failed to create 'repositories' folder.");
      exit(1);
    }
    else
    {
      log_message(SUCCESS, SUCCESS_SYMBOL, "'repositories' folder created successfully.");
    }
  }
  else
  {
    log_message(INFO, INFO_SYMBOL, "'repositories' folder already exists.");
  }
}

void clone_new_repo(const char *repo_id, const char *git_url, const char *destination_folder, const char *branch_name, const char *docker_image_prefix, const char *docker_port)
{
  char command[MAX_PATH_LEN + 512];
  char docker_image_tag[256];
  char backup_folder[MAX_PATH_LEN + 512];
  char rename_message[1024]; // Increase the buffer size to avoid truncation

  // Ensure the "repositories" folder exists
  ensure_repositories_folder_exists();

  // Ensure destination_folder is inside "repositories" folder
  char actual_destination_folder[MAX_PATH_LEN];
  snprintf(actual_destination_folder, sizeof(actual_destination_folder), "repositories/%s", destination_folder);

  // Check if the destination folder exists
  struct stat st;
  if (stat(actual_destination_folder, &st) == 0 && S_ISDIR(st.st_mode))
  {
    // Generate a timestamp for the backup folder name
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int ret = snprintf(backup_folder, sizeof(backup_folder), "%s_backup_%04d%02d%02d_%02d%02d%02d",
                       actual_destination_folder,
                       t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                       t->tm_hour, t->tm_min, t->tm_sec);

    if (ret >= (int)sizeof(backup_folder))
    {
      log_message(ERROR, ERROR_SYMBOL, "Backup folder path is too long. Exiting.");
      exit(1);
    }

    // Rename the existing folder
    if (rename(actual_destination_folder, backup_folder) != 0)
    {
      perror("rename");
      log_message(ERROR, ERROR_SYMBOL, "Failed to rename existing directory.");
      exit(1);
    }
    else
    {
      int rename_message_length = snprintf(rename_message, sizeof(rename_message),
                                           "Existing directory renamed to %s.", backup_folder);
      if (rename_message_length >= (int)sizeof(rename_message))
      {
        log_message(WARNING, WARNING_SYMBOL, "Rename message was truncated.");
      }
      else
      {
        log_message(INFO, INFO_SYMBOL, rename_message);
      }
    }
  }

  // Determine the Docker image tag based on the branch name
  if (strcmp(branch_name, "main") == 0)
  {
    snprintf(docker_image_tag, sizeof(docker_image_tag), "%s:latest", docker_image_prefix);
  }
  else if (strcmp(branch_name, "dev") == 0)
  {
    snprintf(docker_image_tag, sizeof(docker_image_tag), "%s:dev", docker_image_prefix);
  }
  else
  {
    snprintf(docker_image_tag, sizeof(docker_image_tag), "%s:%s", docker_image_prefix, branch_name);
  }

  // Clone the repository
  int ret = snprintf(command, sizeof(command), "git clone -b %s %s %s > /dev/null 2>&1", branch_name, git_url, actual_destination_folder);
  if (ret >= (int)sizeof(command))
  {
    log_message(ERROR, ERROR_SYMBOL, "Command buffer overflow. Exiting.");
    exit(1);
  }
  execute_command(command);

  log_message(SUCCESS, SUCCESS_SYMBOL, "Repository cloned successfully.");

  // Determine the framework
  const char *framework = check_repo_framework(actual_destination_folder);
  char framework_message[256];
  snprintf(framework_message, sizeof(framework_message), "Detected framework: %s", framework);
  log_message(INFO, INFO_SYMBOL, framework_message);

  // Save the repository information to the database, including Docker port
  char sql[MAX_PATH_LEN + 512];
  ret = snprintf(sql, sizeof(sql),
                 "INSERT INTO repositories (id, git_url, destination_folder, branch_name, docker_image_tag, docker_port) "
                 "VALUES ('%s', '%s', '%s', '%s', '%s', '%s');",
                 repo_id, git_url, actual_destination_folder, branch_name, docker_image_tag, docker_port);

  if (ret >= (int)sizeof(sql))
  {
    log_message(ERROR, ERROR_SYMBOL, "SQL query buffer overflow. Exiting.");
    exit(1);
  }

  execute_query(sql);
  log_message(SUCCESS, SUCCESS_SYMBOL, "Repository information saved to database.");
}

void list_repositories()
{
  const char *sql = "SELECT id, git_url, destination_folder, branch_name, docker_image_tag, docker_port FROM repositories;";
  sqlite3_stmt *stmt;

  log_message(INFO, INFO_SYMBOL, "Fetching repository list...");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to fetch repositories.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  log_message(SUCCESS, SUCCESS_SYMBOL, "Repository list fetched successfully.");

  // Adjust the column widths for better precision
  int id_width = 25;
  int git_url_width = 60;
  int dest_folder_width = 20;
  int branch_width = 10;
  int docker_tag_width = 30;
  int docker_port_width = 15;

  // Print the list after logging
  printf("\n%-*s %-*s %-*s %-*s %-*s %-*s\n",
         id_width, "ID",
         git_url_width, "Git URL",
         dest_folder_width, "Destination Folder",
         branch_width, "Branch",
         docker_tag_width, "Docker Image Tag",
         docker_port_width, "Docker Port");

  // Print the separator line based on column widths
  printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
         id_width, "-------------------------",
         git_url_width, "------------------------------------------------------------",
         dest_folder_width, "--------------------",
         branch_width, "----------",
         docker_tag_width, "------------------------------",
         docker_port_width, "--------------");

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const unsigned char *repo_id = sqlite3_column_text(stmt, 0);
    const unsigned char *git_url = sqlite3_column_text(stmt, 1);
    const unsigned char *destination_folder = sqlite3_column_text(stmt, 2);
    const unsigned char *branch_name = sqlite3_column_text(stmt, 3);
    const unsigned char *docker_image_tag = sqlite3_column_text(stmt, 4);
    const unsigned char *docker_port = sqlite3_column_text(stmt, 5);

    printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
           id_width, repo_id,
           git_url_width, git_url,
           dest_folder_width, destination_folder,
           branch_width, branch_name,
           docker_tag_width, docker_image_tag,
           docker_port_width, docker_port);
  }

  sqlite3_finalize(stmt);
}

void pull_latest_repo(const char *repo_id)
{
  sqlite3_stmt *stmt;
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT destination_folder, branch_name FROM repositories WHERE id = ?;");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to prepare statement.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_text(stmt, 1, repo_id, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *destination_folder = (const char *)sqlite3_column_text(stmt, 0);
    const char *branch_or_tag = (const char *)sqlite3_column_text(stmt, 1);

    char command[512];
    char log_msg[256];

    if (branch_or_tag[0] == 'v' || strchr(branch_or_tag, '.') != NULL)
    {
      // It's a version tag
      snprintf(command, sizeof(command), "cd %s && git fetch --tags > /dev/null 2>&1 && git checkout `git describe --tags $(git rev-list --tags --max-count=1)` > /dev/null 2>&1", destination_folder);

      snprintf(log_msg, sizeof(log_msg), "Updating repository %s to the latest version tag...", repo_id);
      log_message(INFO, INFO_SYMBOL, log_msg);

      execute_command(command);

      // Update the database with the latest version tag
      snprintf(sql, sizeof(sql), "UPDATE repositories SET branch_name = (SELECT `git describe --tags $(git rev-list --tags --max-count=1)`), docker_image_tag = ? WHERE id = ?;");
      sqlite3_stmt *update_stmt;

      if (sqlite3_prepare_v2(db, sql, -1, &update_stmt, 0) != SQLITE_OK)
      {
        log_message(ERROR, ERROR_SYMBOL, "Failed to prepare update statement.");
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
      }
      else
      {
        char new_docker_image_tag[256];
        snprintf(new_docker_image_tag, sizeof(new_docker_image_tag), "%s:%s", repo_id, branch_or_tag);

        sqlite3_bind_text(update_stmt, 1, new_docker_image_tag, -1, SQLITE_STATIC);
        sqlite3_bind_text(update_stmt, 2, repo_id, -1, SQLITE_STATIC);

        if (sqlite3_step(update_stmt) != SQLITE_DONE)
        {
          log_message(ERROR, ERROR_SYMBOL, "Failed to update repository information.");
          fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        }
        else
        {
          log_message(SUCCESS, SUCCESS_SYMBOL, "Repository updated to latest version tag.");
        }

        sqlite3_finalize(update_stmt);
      }
    }
    else
    {
      // It's a branch, check for unstaged changes
      snprintf(command, sizeof(command), "cd %s && git diff --quiet --ignore-submodules HEAD", destination_folder);
      int has_unstaged_changes = system(command);

      if (has_unstaged_changes != 0)
      {
        log_message(WARNING, WARNING_SYMBOL, "Unstaged changes detected, stashing changes...");
        snprintf(command, sizeof(command), "cd %s && git stash > /dev/null 2>&1", destination_folder);
        execute_command(command);

        // Check for uncommitted changes after stashing
        snprintf(command, sizeof(command), "cd %s && git diff --quiet --ignore-submodules HEAD", destination_folder);
        int has_uncommitted_changes = system(command);

        if (has_uncommitted_changes != 0)
        {
          log_message(WARNING, WARNING_SYMBOL, "Uncommitted changes detected, restoring working directory...");
          snprintf(command, sizeof(command), "cd %s && git restore --staged . > /dev/null 2>&1", destination_folder);
          execute_command(command);
        }
      }

      // Perform a rebase
      snprintf(command, sizeof(command), "cd %s && git fetch --all > /dev/null 2>&1 && git rebase > /dev/null 2>&1", destination_folder);
      snprintf(log_msg, sizeof(log_msg), "Rebasing branch %s in repository %s...", branch_or_tag, repo_id);
      log_message(INFO, INFO_SYMBOL, log_msg);

      execute_command(command);

      // After rebasing, pull the latest changes
      snprintf(command, sizeof(command), "cd %s && git pull > /dev/null 2>&1", destination_folder);
      snprintf(log_msg, sizeof(log_msg), "Pulling the latest changes for branch %s in repository %s...", branch_or_tag, repo_id);
      log_message(INFO, INFO_SYMBOL, log_msg);

      execute_command(command);

      // Apply stashed changes if any
      if (has_unstaged_changes != 0)
      {
        log_message(INFO, INFO_SYMBOL, "Applying stashed changes...");
        snprintf(command, sizeof(command), "cd %s && git stash pop > /dev/null 2>&1", destination_folder);
        int stash_pop_result = system(command);

        if (stash_pop_result != 0)
        {
          log_message(ERROR, ERROR_SYMBOL, "Merge conflicts detected when applying stashed changes.");
          log_message(WARNING, WARNING_SYMBOL, "Please resolve conflicts manually. The stash entry has been kept.");
          // Exit or return to allow the user to resolve conflicts manually
          return;
        }
      }

      log_message(SUCCESS, SUCCESS_SYMBOL, "Repository updated successfully.");
    }
  }
  else
  {
    log_message(ERROR, ERROR_SYMBOL, "Repository ID not found.");
  }

  sqlite3_finalize(stmt);
}

void pull_all_repos()
{
  const char *sql = "SELECT id FROM repositories;";
  sqlite3_stmt *stmt;

  log_message(INFO, INFO_SYMBOL, "Fetching all repositories to pull latest updates...");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to fetch repository IDs.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *repo_id = (const char *)sqlite3_column_text(stmt, 0);
    pull_latest_repo(repo_id);
  }

  sqlite3_finalize(stmt);
  log_message(SUCCESS, SUCCESS_SYMBOL, "All repositories have been updated.");
}

int validate_docker_image_tag(const char *docker_image_tag)
{
  // Check if the string contains exactly one colon and one or more slashes
  const char *colon = strchr(docker_image_tag, ':');
  const char *slash = strchr(docker_image_tag, '/');

  if (!colon || !slash)
  {
    return 0; // Invalid format
  }

  // Ensure there's text before the colon and before the slash
  if (colon == docker_image_tag || slash == docker_image_tag)
  {
    return 0; // Invalid format
  }

  // Ensure there's text after the colon
  if (*(colon + 1) == '\0')
  {
    return 0; // Invalid format
  }

  return 1; // Valid format
}

void switch_to_branch_or_tag(const char *repo_id, const char *branch_or_tag)
{
  sqlite3_stmt *stmt;
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT destination_folder, docker_image_tag FROM repositories WHERE id = ?;");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to prepare statement.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_text(stmt, 1, repo_id, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *destination_folder = (const char *)sqlite3_column_text(stmt, 0);
    const char *docker_image_prefix = strtok((char *)sqlite3_column_text(stmt, 1), ":");

    char command[512];
    snprintf(command, sizeof(command), "cd %s && git fetch --all > /dev/null 2>&1", destination_folder);

    // Determine if branch_or_tag is a branch or a tag
    char log_msg[256];
    if (strncmp(branch_or_tag, "v", 1) == 0 || strchr(branch_or_tag, '.') != NULL)
    {
      // Assume it's a version tag if it starts with "v" or contains a "."
      snprintf(log_msg, sizeof(log_msg), "Switching %s repository to version tag %s...", repo_id, branch_or_tag);
      snprintf(command + strlen(command), sizeof(command) - strlen(command), " && git checkout tags/%s > /dev/null 2>&1", branch_or_tag);
    }
    else
    {
      // Otherwise, treat it as a branch
      snprintf(log_msg, sizeof(log_msg), "Switching %s repository to branch %s...", repo_id, branch_or_tag);
      snprintf(command + strlen(command), sizeof(command) - strlen(command), " && git checkout %s > /dev/null 2>&1", branch_or_tag);
    }

    log_message(INFO, INFO_SYMBOL, log_msg);

    execute_command(command);
    log_message(SUCCESS, SUCCESS_SYMBOL, "Repository switched successfully.");

    // Update the branch and Docker image tag in the database
    snprintf(sql, sizeof(sql), "UPDATE repositories SET branch_name = ?, docker_image_tag = ? WHERE id = ?;");
    sqlite3_stmt *update_stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &update_stmt, 0) != SQLITE_OK)
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to prepare update statement.");
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      return;
    }

    // Determine the new Docker image tag
    char new_docker_image_tag[256];
    if (strcmp(branch_or_tag, "main") == 0)
    {
      // If switching to main branch, use :latest tag
      snprintf(new_docker_image_tag, sizeof(new_docker_image_tag), "%s:latest", docker_image_prefix);
    }
    else
    {
      // Otherwise, use the branch_or_tag as the tag
      snprintf(new_docker_image_tag, sizeof(new_docker_image_tag), "%s:%s", docker_image_prefix, branch_or_tag);
    }

    sqlite3_bind_text(update_stmt, 1, branch_or_tag, -1, SQLITE_STATIC);
    sqlite3_bind_text(update_stmt, 2, new_docker_image_tag, -1, SQLITE_STATIC);
    sqlite3_bind_text(update_stmt, 3, repo_id, -1, SQLITE_STATIC);

    if (sqlite3_step(update_stmt) != SQLITE_DONE)
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to update repository information.");
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    }
    else
    {
      log_message(SUCCESS, SUCCESS_SYMBOL, "Repository branch or tag and Docker image tag updated successfully.");
    }

    sqlite3_finalize(update_stmt);
  }
  else
  {
    log_message(ERROR, ERROR_SYMBOL, "Repository ID not found.");
  }

  sqlite3_finalize(stmt);
}

void deploy_repo(const char *repo_id)
{
  sqlite3_stmt *stmt;
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT destination_folder, docker_image_tag, docker_port FROM repositories WHERE id = ?;");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to prepare statement.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_text(stmt, 1, repo_id, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *destination_folder = (const char *)sqlite3_column_text(stmt, 0);
    const char *docker_image_tag = (const char *)sqlite3_column_text(stmt, 1);
    const char *docker_port = (const char *)sqlite3_column_text(stmt, 2);

    // Pull the latest changes from the repository
    pull_latest_repo(repo_id);

    // Convert destination_folder to an absolute path
    char absolute_destination_folder[PATH_MAX];
    if (!realpath(destination_folder, absolute_destination_folder))
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to get absolute path of destination folder.");
      sqlite3_finalize(stmt);
      return;
    }

    // Determine the framework
    const char *framework = check_repo_framework(destination_folder);

    int use_php82 = 0;

    if (strcmp(framework, "laravel") == 0)
    {
      // Check Laravel and PHP versions
      use_php82 = check_laravel_and_php_versions(absolute_destination_folder);
    }

    // Dockerfile path selection
    char dockerfile_path[PATH_MAX + 100]; // Allow for extra path length
    char config_source[PATH_MAX + 100];
    char config_destination[PATH_MAX + 100];

    if (strcmp(framework, "laravel") == 0)
    {
      if (use_php82)
      {
        snprintf(dockerfile_path, sizeof(dockerfile_path), "%s/docker/php82.dockerfile", absolute_destination_folder);
      }
      else
      {
        snprintf(dockerfile_path, sizeof(dockerfile_path), "%s/docker/Dockerfile", absolute_destination_folder);
      }
      snprintf(config_source, sizeof(config_source), "config/laravel");
    }
    else if (strcmp(framework, "static-php") == 0)
    {
      snprintf(dockerfile_path, sizeof(dockerfile_path), "%s/docker/Dockerfile", absolute_destination_folder);
      snprintf(config_source, sizeof(config_source), "config/static-php");
    }
    else
    {
      log_message(ERROR, ERROR_SYMBOL, "Unknown framework. Deployment aborted.");
      sqlite3_finalize(stmt);
      return;
    }

    // Copy config files into the destination folder's docker directory
    snprintf(config_destination, sizeof(config_destination), "%s/docker", absolute_destination_folder);

    mkdir(config_destination, 0700);

    char copy_command[1024];
    int ret = snprintf(copy_command, sizeof(copy_command), "cp -r %s/* %s/", config_source, config_destination);

    if (ret < 0 || ret >= (int)sizeof(copy_command))
    {
      fprintf(stderr, "Copy command buffer overflow. Deployment aborted.\n");
      sqlite3_finalize(stmt);
      return;
    }

    ret = system(copy_command);
    if (ret != 0)
    {
      fprintf(stderr, "Failed to copy config files with exit code %d: %s\n", WEXITSTATUS(ret), copy_command);
      sqlite3_finalize(stmt);
      exit(1);
    }

    // Get current user's UID and GID
    char uid_str[16];
    char gid_str[16];
    snprintf(uid_str, sizeof(uid_str), "%d", getuid());
    snprintf(gid_str, sizeof(gid_str), "%d", getgid());

    // Build the Docker image with HOST_UID and HOST_GID as build arguments
    char build_command[1024];
    ret = snprintf(build_command, sizeof(build_command),
                   "docker build --build-arg HOST_UID=%s --build-arg HOST_GID=%s -t %s -f %s %s > /dev/null 2>&1",
                   uid_str, gid_str, docker_image_tag, dockerfile_path, absolute_destination_folder);

    if (ret < 0 || ret >= (int)sizeof(build_command))
    {
      log_message(ERROR, ERROR_SYMBOL, "Command buffer overflow. Deployment aborted.");
      sqlite3_finalize(stmt);
      return;
    }

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Deploying %s repository with framework: %s", repo_id, framework);
    log_message(INFO, INFO_SYMBOL, log_msg);

    ret = system(build_command);
    if (ret != 0)
    {
      fprintf(stderr, "Docker build failed with exit code %d: %s\n", WEXITSTATUS(ret), build_command);
      sqlite3_finalize(stmt);
      exit(1);
    }
    log_message(SUCCESS, SUCCESS_SYMBOL, "Docker image built successfully.");

    // Check if the service already exists
    char service_check_command[256];
    snprintf(service_check_command, sizeof(service_check_command), "docker service ls --filter name=%s_service --format '{{.Name}}'", repo_id);

    FILE *service_check = popen(service_check_command, "r");
    if (!service_check)
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to check existing Docker service.");
      sqlite3_finalize(stmt);
      return;
    }

    char service_name[256];
    if (fgets(service_name, sizeof(service_name), service_check) != NULL)
    {
      // Service exists, update it with rolling update strategy
      pclose(service_check);
      char update_command[1024];
      ret = snprintf(update_command, sizeof(update_command),
                     "docker service update --force --publish-add %s --mount-add type=bind,source=%s,target=/app "
                     "--update-delay 10s --update-parallelism 2 --with-registry-auth %s_service > /dev/null 2>&1",
                     docker_port, absolute_destination_folder, repo_id);
      if (ret < 0 || ret >= (int)sizeof(update_command))
      {
        log_message(ERROR, ERROR_SYMBOL, "Update command buffer overflow. Deployment aborted.");
        sqlite3_finalize(stmt);
        return;
      }

      ret = system(update_command);
      if (ret != 0)
      {
        fprintf(stderr, "Docker service update failed with exit code %d: %s\n", WEXITSTATUS(ret), update_command);
        sqlite3_finalize(stmt);
        exit(1);
      }
      log_message(SUCCESS, SUCCESS_SYMBOL, "Docker service updated successfully.");
    }
    else
    {
      // Service does not exist, create it
      pclose(service_check);
      char create_command[1024];
      ret = snprintf(create_command, sizeof(create_command),
                     "docker service create --name %s_service --replicas 1 --publish %s --mount type=bind,source=%s,target=/app "
                     "--update-delay 10s --update-parallelism 2 --with-registry-auth %s > /dev/null 2>&1",
                     repo_id, docker_port, absolute_destination_folder, docker_image_tag);

      if (ret < 0 || ret >= (int)sizeof(create_command))
      {
        log_message(ERROR, ERROR_SYMBOL, "Create command buffer overflow. Deployment aborted.");
        sqlite3_finalize(stmt);
        return;
      }

      ret = system(create_command);
      if (ret != 0)
      {
        fprintf(stderr, "Docker service create failed with exit code %d: %s\n", WEXITSTATUS(ret), create_command);
        sqlite3_finalize(stmt);
        exit(1);
      }
      log_message(SUCCESS, SUCCESS_SYMBOL, "Docker service created successfully.");
    }

    // Remove the docker directory after successful deployment
    char remove_command[1024];
    ret = snprintf(remove_command, sizeof(remove_command), "rm -rf %s/docker > /dev/null 2>&1", absolute_destination_folder);
    if (ret < 0 || ret >= (int)sizeof(remove_command))
    {
      log_message(WARNING, WARNING_SYMBOL, "Remove command buffer overflow. Directory not removed.");
    }
    else
    {
      ret = system(remove_command);
      if (ret != 0)
      {
        log_message(WARNING, WARNING_SYMBOL, "Failed to remove docker directory after deployment.");
      }
      else
      {
        log_message(SUCCESS, SUCCESS_SYMBOL, "Docker directory removed successfully after deployment.");
      }
    }

    // Clean up dangling images and unused resources
    clean_up_unused_resources();
  }
  else
  {
    log_message(ERROR, ERROR_SYMBOL, "Repository ID not found.");
  }

  sqlite3_finalize(stmt);
}

void deploy_all_repos()
{
  const char *sql = "SELECT id FROM repositories;";
  sqlite3_stmt *stmt;

  log_message(INFO, INFO_SYMBOL, "Deploying all repositories...");

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
  {
    log_message(ERROR, ERROR_SYMBOL, "Failed to fetch repository IDs.");
    fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
    return;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    const char *repo_id = (const char *)sqlite3_column_text(stmt, 0);
    deploy_repo(repo_id);
  }

  sqlite3_finalize(stmt);
  log_message(SUCCESS, SUCCESS_SYMBOL, "All repositories have been deployed.");
}