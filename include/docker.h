#ifndef DOCKER_H
#define DOCKER_H

#include <stdlib.h>

// Function declarations for Docker-related operations
void clean_up_unused_resources();
void show_docker_service_logs(const char *repo_id);

#endif // DOCKER_H