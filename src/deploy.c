#include "logger.h"
#include "database.h"
#include "utils.h"
#include "docker.h"
#include "repo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

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

    // Get the HOME environment variable
    const char *home_dir = getenv("HOME");
    if (!home_dir)
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to get home directory.");
      sqlite3_finalize(stmt);
      return;
    }

    // Construct the base path to the config directory in {HOME}/.config/dployer
    char config_base[PATH_MAX];
    if (snprintf(config_base, sizeof(config_base), "%s/.config/dployer/config", home_dir) >= sizeof(config_base))
    {
      log_message(ERROR, ERROR_SYMBOL, "Config base directory path is too long.");
      sqlite3_finalize(stmt);
      return;
    }

    // Determine the correct config source directory based on the framework
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
      snprintf(config_source, sizeof(config_source), "%s/laravel", config_base);
    }
    else if (strcmp(framework, "static-php") == 0)
    {
      snprintf(dockerfile_path, sizeof(dockerfile_path), "%s/docker/Dockerfile", absolute_destination_folder);
      snprintf(config_source, sizeof(config_source), "%s/static-php", config_base);
    }
    else
    {
      log_message(ERROR, ERROR_SYMBOL, "Unknown framework. Deployment aborted.");
      sqlite3_finalize(stmt);
      return;
    }

    // Ensure the docker directory exists in the destination folder
    snprintf(config_destination, sizeof(config_destination), "%s/docker", absolute_destination_folder);
    if (mkdir(config_destination, 0700) == -1 && errno != EEXIST)
    {
      log_message(ERROR, ERROR_SYMBOL, "Failed to create docker directory in the destination.");
      sqlite3_finalize(stmt);
      return;
    }

    // Copy config files into the destination folder's docker directory
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

void delete_service(const char *repo_id)
{
  // Ensure repo_id is correctly handled
  if (repo_id == NULL || strlen(repo_id) == 0)
  {
    log_message(ERROR, ERROR_SYMBOL, "Invalid repository ID.");
    return;
  }

  // Construct the Docker service removal command
  char service_delete_command[512];
  snprintf(service_delete_command, sizeof(service_delete_command), "docker service rm %s_service > /dev/null 2>&1", repo_id);

  int ret = system(service_delete_command);
  if (ret != 0)
  {
    fprintf(stderr, "Failed to delete Docker service with exit code %d: %s\n", WEXITSTATUS(ret), service_delete_command);
  }
  else
  {
    log_message(SUCCESS, SUCCESS_SYMBOL, "Docker service deleted successfully.");
  }

  // Delete the repository entry and its directory
  delete_repo(repo_id);
}