#include "docker.h"
#include "logger.h"

void clean_up_unused_resources()
{
    log_message(INFO, INFO_SYMBOL, "Cleaning up dangling images and unused resources...");

    // Remove dangling images
    const char *remove_dangling_images_command = "docker image prune -f > /dev/null 2>&1";
    int ret = system(remove_dangling_images_command);
    if (ret != 0)
    {
        log_message(WARNING, WARNING_SYMBOL, "Failed to remove dangling images.");
    }
    else
    {
        log_message(SUCCESS, SUCCESS_SYMBOL, "Dangling images removed successfully.");
    }

    // Remove unused networks
    const char *remove_unused_networks_command = "docker network prune -f > /dev/null 2>&1";
    ret = system(remove_unused_networks_command);
    if (ret != 0)
    {
        log_message(WARNING, WARNING_SYMBOL, "Failed to remove unused networks.");
    }
    else
    {
        log_message(SUCCESS, SUCCESS_SYMBOL, "Unused networks removed successfully.");
    }

    // Remove unused volumes
    const char *remove_unused_volumes_command = "docker volume prune -f > /dev/null 2>&1";
    ret = system(remove_unused_volumes_command);
    if (ret != 0)
    {
        log_message(WARNING, WARNING_SYMBOL, "Failed to remove unused volumes.");
    }
    else
    {
        log_message(SUCCESS, SUCCESS_SYMBOL, "Unused volumes removed successfully.");
    }

    log_message(SUCCESS, SUCCESS_SYMBOL, "Cleanup completed.");
}

void show_docker_service_logs(const char *repo_id)
{
    char command[512];
    snprintf(command, sizeof(command), "docker service logs -f %s_service", repo_id);

    log_message(INFO, INFO_SYMBOL, "Fetching and following Docker service logs...");

    int ret = system(command);

    if (ret == -1)
    {
        perror("system");
        exit(1);
    }
    else if (ret != 0)
    {
        // Handle non-zero return codes
        fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(ret), command);

        // Optionally, you can re-run the command without redirection for debugging:
        char debug_command[512];
        snprintf(debug_command, sizeof(debug_command), "%s", command);
        system(debug_command); // Run the command again to see the output
        exit(1);
    }

    log_message(SUCCESS, SUCCESS_SYMBOL, "Docker service logs displayed successfully.");
}