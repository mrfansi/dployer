#include "utils.h"
#include "logger.h"
#include <pthread.h>
#include <unistd.h>

extern int loading; // Assuming 'loading' is declared in another file (like main.c)

// Loader animation function
void *loader_animation(void *arg)
{
  (void)arg;

  const char *spinner = "|/-\\";
  int i = 0;

  while (loading)
  {
    printf("\r"); // Return the cursor to the beginning of the line
    char message[256];
    snprintf(message, sizeof(message), "[%c] Processing...", spinner[i % 4]);
    printf("%s[LOG] INFO [%04d-%02d-%02d %02d:%02d:%02d] %s%s",
           INFO,
           2024, 8, 13, 15, 7, 29, // Placeholder for the timestamp; you should replace with actual time
           message, NC);
    fflush(stdout);
    i++;
    usleep(100000); // Sleep for 100ms
  }

  // Clear the line by overwriting with spaces and return to the beginning
  printf("\r%*s\r", 80, ""); // Assumes 80 characters wide; adjust if needed
  log_message(SUCCESS, SUCCESS_SYMBOL, "[✔] Done!");

  pthread_exit(NULL);
}

// Function to get input from the user
void get_input(const char *prompt, char *input, size_t size)
{
  printf("\033[1;33m%s %s \033[0m", INPUT_SYMBOL, prompt); // Yellow prompt text with cyan arrow
  fgets(input, size, stdin);

  // Remove the trailing newline character
  size_t len = strlen(input);
  if (len > 0 && input[len - 1] == '\n')
  {
    input[len - 1] = '\0';
  }
}

// Function to execute a command in the system shell
void execute_command(const char *command)
{
  // Log the command being executed
  char log_msg[512];
  snprintf(log_msg, sizeof(log_msg), "Executing command: %s", command);
  log_message(INFO, INFO_SYMBOL, log_msg);

  // Create a thread for the loader animation
  pthread_t loader_thread;
  loading = 1; // Set the loading flag to true
  pthread_create(&loader_thread, NULL, loader_animation, NULL);

  int ret = system(command);
  loading = 0; // Stop the loader

  // Wait for the loader thread to finish
  pthread_join(loader_thread, NULL);

  if (ret == -1)
  {
    perror("system");
    exit(1);
  }
  else if (ret != 0)
  {
    // Handle non-zero return codes
    fprintf(stderr, "Command failed with exit code %d: %s\n", WEXITSTATUS(ret), command);

    // Remove redirection to /dev/null for debugging
    char debug_command[512];
    snprintf(debug_command, sizeof(debug_command), "%s", command);
    for (char *p = debug_command; *p; p++)
    {
      if (strncmp(p, "> /dev/null 2>&1", 16) == 0)
      {
        *p = '\0'; // Truncate at redirection
        break;
      }
    }
    system(debug_command); // Run the command again without redirection to see the output
    exit(1);
  }
}

void check_requirements()
{

  // Check if Git is installed
  if (system("command -v git > /dev/null") != 0)
  {
    log_message(ERROR, ERROR_SYMBOL, "Git is not installed. Please install Git.");
    exit(1);
  }

  // Check if Docker is installed
  if (system("command -v docker > /dev/null") != 0)
  {
    log_message(ERROR, ERROR_SYMBOL, "Docker is not installed. Please install Docker.");
    exit(1);
  }

  // Initialize Docker Swarm
  int swarm_init = system("docker swarm init > /dev/null 2>&1");
  if (swarm_init != 0)
  {
    // Check if the swarm is already active
    if (system("docker node ls > /dev/null 2>&1") != 0)
    {
      log_message(WARNING, WARNING_SYMBOL, "Failed to initialize Docker Swarm. It might already be initialized.");
    }
    // Swarm is already active; no warning needed
  }
  else
  {
    log_message(SUCCESS, SUCCESS_SYMBOL, "Docker Swarm initialized successfully.");
  }
}