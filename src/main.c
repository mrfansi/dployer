#include "logger.h"
#include "database.h"
#include "repo.h"
#include "docker.h"
#include "deploy.h"
#include "utils.h"

int loading = 0; // Global variable to control the loader

void print_banner()
{
    printf("\n");
    printf(" ____        _                       \n");
    printf("|  _ \\ _ __ | | ___  _   _  ___ _ __ \n");
    printf("| | | | '_ \\| |/ _ \\| | | |/ _ \\ '__|\n");
    printf("| |_| | |_) | | (_) | |_| |  __/ |   \n");
    printf("|____/| .__/|_|\\___/ \\__, |\\___|_|   \n");
    printf("      |_|            |___/           \n");
    printf("\n");
    printf("          Built with love            \n");
    printf("          by Muhammad Irfan          \n");
    printf("\n");
}

void print_help()
{
    printf("Available commands:\n");
    printf("  new, n                                              - Create a new repository entry\n");
    printf("  list, l                                             - List all repositories\n");
    printf("  update, u                                           - Update all repositories\n");
    printf("  update <ID>, u <ID>                                 - Update a specific repository by ID\n");
    printf("  switch <ID> <BRANCH_OR_TAG>, s <ID> <BRANCH_OR_TAG> - Switch to a specific branch or tag for a repository\n");
    printf("  deploy, d                                           - Deploy all repositories\n");
    printf("  deploy <ID>, dep <ID>                               - Deploy a specific repository by ID\n");
    printf("  delete <ID>, del <ID>                               - Delete a repository and its Docker service by ID\n"); // Fixed closing quote
    printf("  exit, quit, q                                       - Exit the mini terminal\n");
    printf("  help, h                                             - Show this help message\n");
    printf("\n");
}

void mini_terminal()
{
    char command[256];

    while (1)
    {
        printf("[command]$ "); // Prompt
        fgets(command, sizeof(command), stdin);

        // Remove trailing newline character
        command[strcspn(command, "\n")] = 0;

        // Handle different commands
        if (strcmp(command, "new") == 0 || strcmp(command, "n") == 0)
        {
            char repo_id[128];
            char git_url[256];
            char destination_folder[256];
            char branch_name[128];
            char docker_image_prefix[128];
            char docker_port[16];

            get_input("Enter the repository ID (a unique string identifier):", repo_id, sizeof(repo_id));
            get_input("Enter the Git repository URL:", git_url, sizeof(git_url));
            get_input("Enter the destination folder (relative to 'repositories' folder):", destination_folder, sizeof(destination_folder));
            get_input("Enter the branch name (default: main):", branch_name, sizeof(branch_name));
            get_input("Enter the Docker image prefix (format: username/image):", docker_image_prefix, sizeof(docker_image_prefix));
            get_input("Enter the Docker port to expose (format: host_port:container_port):", docker_port, sizeof(docker_port));

            if (strlen(branch_name) == 0)
            {
                strcpy(branch_name, "main");
            }

            clone_new_repo(repo_id, git_url, destination_folder, branch_name, docker_image_prefix, docker_port);
        }
        else if (strcmp(command, "list") == 0 || strcmp(command, "l") == 0)
        {
            list_repositories();
        }
        else if (strcmp(command, "update") == 0 || strcmp(command, "u") == 0)
        {
            pull_all_repos();
        }
        else if (strncmp(command, "update ", 7) == 0 || strncmp(command, "u ", 2) == 0)
        {
            const char *repo_id = (command[0] == 'u') ? command + 2 : command + 7;
            pull_latest_repo(repo_id);
        }
        else if (strncmp(command, "switch ", 7) == 0 || strncmp(command, "s ", 2) == 0)
        {
            char *repo_id = strtok((command[0] == 's') ? command + 2 : command + 7, " ");
            char *branch_or_tag = strtok(NULL, " ");
            if (repo_id && branch_or_tag)
            {
                switch_to_branch_or_tag(repo_id, branch_or_tag);
            }
            else
            {
                log_message(WARNING, WARNING_SYMBOL, "Usage: switch <ID> <BRANCH_OR_TAG>");
            }
        }
        else if (strncmp(command, "deploy ", 7) == 0 || strncmp(command, "dep ", 4) == 0)
        {
            const char *repo_id;
            if (strncmp(command, "dep ", 4) == 0)
            {
                repo_id = command + 4;
            }
            else
            {
                repo_id = command + 7;
            }

            // Remove any leading spaces
            while (*repo_id == ' ')
            {
                repo_id++;
            }

            if (repo_id != NULL && strlen(repo_id) > 0)
            {
                char repo_message[256];
                snprintf(repo_message, sizeof(repo_message), "Deploying repository '%s'...", repo_id);
                log_message(SUCCESS, SUCCESS_SYMBOL, repo_message);
                deploy_repo(repo_id);
            }
            else
            {
                log_message(WARNING, WARNING_SYMBOL, "Invalid repository ID. Usage: deploy <ID>");
            }
        }
        else if (strcmp(command, "deploy") == 0 || strcmp(command, "dep") == 0)
        {
            deploy_all_repos();
        }
        else if (strncmp(command, "delete ", 7) == 0 || strncmp(command, "del ", 4) == 0)
        {
            const char *repo_id = (command[0] == 'd' && command[1] == 'e' && command[2] == 'l') ? command + 4 : command + 7;
            delete_service(repo_id);
        }
        else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0)
        {
            print_help(); // Show available commands
        }
        else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0 || strcmp(command, "q") == 0)
        {
            break; // Exit the terminal
        }
        else
        {
            log_message(WARNING, WARNING_SYMBOL, "Unknown command. Type 'help' for a list of commands.");
        }
    }
}

int main(int argc, char *argv[])
{
    print_banner(); // Display the banner at the start
    print_help();   // Display available commands before starting the terminal

    // Check system requirements (e.g., Git and Docker installation)
    check_requirements();

    // Open and initialize the SQLite database in the config folder
    open_database("repositories.db");

    // Start the mini terminal
    mini_terminal();

    // Close the database before exiting
    close_database();

    return 0;
}