#ifndef REPO_H
#define REPO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

// Function declarations related to repository management
void ensure_repositories_folder_exists();
void clone_new_repo(const char *repo_id, const char *git_url, const char *destination_folder, const char *branch_name, const char *docker_image_prefix, const char *docker_port);
void list_repositories();
void pull_latest_repo(const char *repo_id);
void pull_all_repos();
void switch_to_branch_or_tag(const char *repo_id, const char *branch_or_tag);
void deploy_repo(const char *repo_id);
void deploy_all_repos();
const char *check_repo_framework(const char *repo_path);
int check_laravel_and_php_versions(const char *repo_path); // Add this line

#endif // REPO_H