#include "logger.h"
#include "database.h"
#include "repo.h"
#include "docker.h"
#include "utils.h"

int loading = 0; // Global variable to control the loader

int main(int argc, char *argv[])
{
    char *base_dir = getenv("PWD");
    if (!base_dir)
    {
        log_message(ERROR, ERROR_SYMBOL, "Failed to get base directory.");
        return 1;
    }

    // Check system requirements (e.g., Git and Docker installation)
    check_requirements();

    // Open and initialize the SQLite database in the config folder
    open_database("repositories.db");

    if (argc == 1 || strcmp(argv[1], "all") == 0)
    {
        log_message(WARNING, WARNING_SYMBOL, "Please provide a valid command. Use './dployer new' to create a new repository, './dployer list' to list all repositories, './dployer update' to update all repositories, './dployer update <ID>' to update a specific repository, or './dployer switch <ID> <BRANCH_OR_TAG>' to switch to a specific branch or version tag.");
    }
    else if (strcmp(argv[1], "new") == 0)
    {
        char repo_id[128];
        char git_url[256];
        char destination_folder[256];
        char branch_name[128];
        char docker_image_prefix[128];
        char docker_port[16]; // Add this variable to hold the Docker port

        // Prompt the user for input with beautiful prompts
        get_input("Enter the repository ID (a unique string identifier):", repo_id, sizeof(repo_id));
        get_input("Enter the Git repository URL:", git_url, sizeof(git_url));
        get_input("Enter the destination folder (relative to 'repositories' folder):", destination_folder, sizeof(destination_folder));
        get_input("Enter the branch name (default: main):", branch_name, sizeof(branch_name));
        get_input("Enter the Docker image prefix (format: username/image):", docker_image_prefix, sizeof(docker_image_prefix));
        get_input("Enter the Docker port to expose (format: host_port:container_port):", docker_port, sizeof(docker_port)); // Prompt for Docker port

        // Set default values if necessary
        if (strlen(branch_name) == 0)
        {
            strcpy(branch_name, "main");
        }

        // Clone the repository and save information to the database, including Docker port
        clone_new_repo(repo_id, git_url, destination_folder, branch_name, docker_image_prefix, docker_port);
    }
    else if (strcmp(argv[1], "list") == 0)
    {
        // List all repositories
        list_repositories();
    }
    else if (strcmp(argv[1], "logs") == 0)
    {
        if (argc == 3)
        {
            // Display Docker service logs for a specific repository
            const char *repo_id = argv[2];
            show_docker_service_logs(repo_id);
        }
        else
        {
            log_message(WARNING, WARNING_SYMBOL, "Usage: ./dployer logs <ID>");
            return 1;
        }
    }
    else if (strcmp(argv[1], "update") == 0)
    {
        if (argc == 2)
        {
            // Update all repositories when no ID is provided
            pull_all_repos();
        }
        else if (argc == 3)
        {
            // If "update <ID>" is provided, update the specific repository
            const char *repo_id = argv[2];
            pull_latest_repo(repo_id);
        }
        else
        {
            log_message(WARNING, WARNING_SYMBOL, "Usage: ./dployer update [ID]");
            return 1;
        }
    }
    else if (strcmp(argv[1], "switch") == 0)
    {
        if (argc == 4)
        {
            // Switch to a specific branch or version tag
            const char *repo_id = argv[2];
            const char *branch_or_tag = argv[3];
            switch_to_branch_or_tag(repo_id, branch_or_tag);
        }
        else
        {
            log_message(WARNING, WARNING_SYMBOL, "Usage: ./dployer switch <ID> <BRANCH_OR_TAG>");
            return 1;
        }
    }
    else if (strcmp(argv[1], "deploy") == 0)
    {
        if (argc == 2)
        {
            // If "deploy" is provided with no ID, deploy all repositories
            deploy_all_repos();
        }
        else if (argc == 3)
        {
            // If "deploy <ID>" is provided, deploy the specific repository
            const char *repo_id = argv[2];
            deploy_repo(repo_id);
        }
        else
        {
            log_message(WARNING, WARNING_SYMBOL, "Usage: ./dployer deploy [ID]");
            return 1;
        }
    }
    else
    {
        log_message(WARNING, WARNING_SYMBOL, "Unknown command.");
        fprintf(stderr, "Usage: %s {new|list|update [ID]|switch <ID> <BRANCH_OR_TAG>|deploy [ID]}\n", argv[0]);
        return 1;
    }

    // Close the database
    close_database();

    return 0;
}