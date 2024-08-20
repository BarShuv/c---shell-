#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // For file operations
#include <signal.h> // For signal handling
#include "LineParser.h"

#define MAX_INPUT_SIZE 2048

int debug = 0;  // Global variable to track the debug flag

void execute(cmdLine *pCmdLine) {
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) {
        // Handle "cd" command internally
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(pCmdLine->arguments[1]) != 0) {
                perror("cd failed");
            }
        }
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "alarm") == 0) {
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "alarm: missing argument\n");
        } else {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGCONT) == 0) {
                printf("Sent SIGCONT to process %d\n", pid);
            } else {
                perror("kill (SIGCONT) failed");
            }
        }
        return;
    }

    if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "blast: missing argument\n");
        } else {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGKILL) == 0) {
                printf("Sent SIGKILL to process %d\n", pid);
            } else {
                perror("kill (SIGKILL) failed");
            }
        }
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  // Child process
        // Redirect input if necessary
        if (pCmdLine->inputRedirect != NULL) {
            int fd_in = open(pCmdLine->inputRedirect, O_RDONLY);
            if (fd_in == -1) {
                perror("open input file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        // Redirect output if necessary
        if (pCmdLine->outputRedirect != NULL) {
            int fd_out = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd_out == -1) {
                perror("open output file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        // Execute the command
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {
            perror("execvp failed");
            _exit(EXIT_FAILURE);
        }
    } else {  // Parent process
        if (pCmdLine->blocking) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            if (debug) {
                fprintf(stderr, "Command '%s' is running in the background\n", pCmdLine->arguments[0]);
            }
        }
    }
}

int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];
    char cwd[PATH_MAX];
    cmdLine *parsedLine;

    // Check for the -d flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
    }

    while (1) {
        // Display prompt
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
        } else {
            perror("getcwd() error");
            return 1;
        }

        // Read input
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        // Remove newline character from input
        input[strcspn(input, "\n")] = '\0';

        // Parse input
        parsedLine = parseCmdLines(input);
        if (parsedLine == NULL) {
            continue;
        }

        // Check for "quit" command
        if (strcmp(parsedLine->arguments[0], "quit") == 0) {
            freeCmdLines(parsedLine);
            break;
        }

        // Execute the command
        execute(parsedLine);

        // Free parsed command line
        freeCmdLines(parsedLine);
    }

    return 0;
}
