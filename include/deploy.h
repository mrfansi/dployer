#ifndef DEPLOY_H
#define DEPLOY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

// Function declarations related to repository management
void deploy_repo(const char *repo_id);
void deploy_all_repos();

#endif // DEPLOY_H