#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "LineParser.h"
#include <bits/waitflags.h>
#include <linux/limits.h>

#define MAX_INPUT_SIZE 2048
#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
int debug = 0;  //global variable to track the debug flag

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;
process *list = NULL;

#define MAX_HISTORY_SIZE 100

typedef struct history_node {
    char* cmd;
    struct history_node* next;
} history_node;

history_node* history_head = NULL;
history_node* history_tail = NULL;
int history_size = 0;


void add_history_node(const char* cmd) {
    history_node *node = malloc(sizeof(history_node));
    if (node == NULL) {
        printf("Error: Failed to allocate memory for history node\n");
        return;
    }

    char *cmd_with_new_line = malloc(strlen(cmd) + 2);
    if (cmd_with_new_line == NULL) {
        printf("Error: Failed to allocate memory for command string\n");
        free(node);
        return;
    }
    sprintf(cmd_with_new_line, "%s\n", cmd);

    node->cmd = cmd_with_new_line;
    node->next = NULL;

    // add new history node to the  list
    if (history_head == NULL) {
        history_head = node;
        history_tail = node;
    } else {
        history_tail->next = node;
        history_tail = node;

        if (history_size >= MAX_HISTORY_SIZE) {
            history_node *old_node = history_head;
            history_head = history_head->next;
            free(old_node->cmd);
            free(old_node);
        } else {
            history_size++;
        }
    }
}


void print_history_node(history_node *history) {
    history_node *current = history;
    int count = 1;
    while (current != NULL) {
        printf("%d. %s\n", count, current->cmd);
        current = current->next;
        count++;
    }
}

const char * print_history_node_on_index(int i) {
    history_node* current = history_head;
    int count = 1;

    while (current != NULL) {
        if (count == i) {
            printf("%s\n", current->cmd);
            return current->cmd;
        }
        count++;
        current = current->next;
    }

    printf("Error: index out of bounds\n");
    return NULL;

}


void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process *new_proc = malloc(sizeof(process));
    new_proc->cmd = cmd;
    new_proc->pid = pid;
    new_proc->status = RUNNING;
    new_proc->next = NULL;
    //if the list is empty set it to the process
    if (*process_list == NULL)
    {
        *process_list = new_proc;
        return;
    }
    //add the process to the end of the list
    process *head = *process_list;
    while (head->next != NULL)
    {
        head = head->next;
    }
    head->next = new_proc;
}
void freeProcessList(process *process_list) {
    //free memory  all nodes of process list
    while (process_list != NULL) {
        process *head = process_list;
        process_list = head->next;
        freeCmdLines(head->cmd);
        free(head);
    }
}


void updateProcessStatus(process* process_list, int pid, int status){
    while (process_list != NULL)
    {
        //search for the node and update status
        if (process_list->pid == pid){
            process_list->status = status;
            return;
        }
        process_list = process_list->next;
    }
}


void updateProcessList(process **process_list){
    process *head = *process_list;
    while (head != NULL)
    {
        int proc_status;
        //not blocking
        pid_t pid = waitpid(head->pid, &proc_status, WNOHANG | WUNTRACED | WCONTINUED);
        //update status depends on waitpid result
        if (pid == -1) {
            head->status = TERMINATED;
        }  else if (WIFSTOPPED(proc_status)) {
            head->status = SUSPENDED;
        } else if (WIFCONTINUED(proc_status)) {
            head->status = RUNNING;
        }
        head = head->next;
    }
}


void printProcessList(process** process_list){
    process *prev = NULL;
    process *head = *process_list;
    int index = 0; //index in the process list

    while (head != NULL)
    {
        char* status;
        int stat = head->status;
        if(stat == -1)
            status = "TERMINATED";
        else if(stat == 0)
            status = "SUSPENDED";
        else if(stat == 1)
            status = "RUNNING";
        else
            status = "UNKNOWN";

        //print in format of: <index in process list> <process id> <process status> <the command together with its arguments>
        printf("%d %d %s %s", index, head->pid, status, head->cmd->arguments[0]);

        //print the command arguments
        for (int i = 1; head->cmd->arguments[i] != NULL; i++) {
            printf(" %s", head->cmd->arguments[i]);
        }
        printf("\n");
        //remove terminated processes
        if (head->status == TERMINATED){
            if (prev != NULL){
                prev->next = head->next;
            }
            else{
                *process_list = head->next;
            }
            freeCmdLines(head->cmd);
            free(head);
        }
        else{
            prev = head;
        }

        head = head->next;
        index++;
    }
}

void handle_sigint(int sig) {
    printf("\nCaught signal %d (SIGINT). Use 'quit' to exit the shell.\n", sig);
}

void execute_single_command(cmdLine *pCmdLine) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  //child process case
        //redirect input if necessary
        if (pCmdLine->inputRedirect != NULL) {
            if (debug) fprintf(stderr, "Redirecting input from: %s\n", pCmdLine->inputRedirect);
            int fd_in = open(pCmdLine->inputRedirect, O_RDONLY);
            if (fd_in == -1) {
                perror("open input file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        //redirect output if necessary
        if (pCmdLine->outputRedirect != NULL) {
            if (debug) fprintf(stderr, "Redirecting output to: %s\n", pCmdLine->outputRedirect);
            FILE *fd_out = fopen(pCmdLine->outputRedirect, "w");
            if (fd_out == NULL) {
                perror("open output file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fileno(fd_out), STDOUT_FILENO);
            fclose(fd_out);
        }

        //execute the command
        if (debug) {
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
            for (int i = 0; i < pCmdLine->argCount; i++) {
                fprintf(stderr, "arg[%d]: %s\n", i, pCmdLine->arguments[i]);
            }
        }
        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1) {

            perror("execvp failed");
            _exit(EXIT_FAILURE);
        }
    } else {  //parent process case
        if (debug) fprintf(stderr, "Created child process with PID: %d\n", pid);
        addProcess(&list, pCmdLine, pid);
        if (pCmdLine->blocking) {
            if (debug) fprintf(stderr, "Waiting for child process %d to complete...\n", pid);
            int status;
            waitpid(pid, &status, 0);
            updateProcessStatus(list, pid, WIFEXITED(status) ? TERMINATED : RUNNING);
        }
    }
}


void execute_pipe_commands(cmdLine *pCmdLine) {
    if (pCmdLine->outputRedirect != NULL) {
        fprintf(stderr, "Error: output redirection on the left-hand side of the pipe is not allowed\n");
        return;
    }
    if (pCmdLine->next->inputRedirect != NULL) {
        fprintf(stderr, "Error: input redirection on the right-hand side of the pipe is not allowed\n");
        return;
    }

    int pipefd[2];
    pid_t cpid1, cpid2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    cpid1 = fork();
    if (cpid1 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (cpid1 == 0) {  //child 1
        close(STDOUT_FILENO);             //close standard output
        dup2(pipefd[1], STDOUT_FILENO);   //duplicate the write end of the pipe to standard output
        close(pipefd[1]);                 // close the original write end of the pipe
        close(pipefd[0]);                 //close the read end of the pipe

        //redirect input if necessary
        if (pCmdLine->inputRedirect != NULL) {
            int fd_in = open(pCmdLine->inputRedirect, O_RDONLY);
            if (fd_in == -1) {
                perror("open input file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (debug) {
            fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
            fprintf(stderr, "(child1>going to execute cmd: %s)\n", pCmdLine->arguments[0]);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments); // Execute left-hand command
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    cpid2 = fork();
    if (cpid2 == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (cpid2 == 0) {  // child 2
        close(STDIN_FILENO);              //close standard input
        dup2(pipefd[0], STDIN_FILENO);    //duplicate the read end of the pipe to standard input
        close(pipefd[0]);                 //close the original read end of the pipe
        close(pipefd[1]);                 //close the write end of the pipe

        //redirect output if necessary
        if (pCmdLine->next->outputRedirect != NULL) {
            FILE* fd_out = fopen(pCmdLine->next->outputRedirect, "w");
            if (fd_out == NULL) {
                perror("open output file failed");
                _exit(EXIT_FAILURE);
            }
            dup2(fileno(fd_out), STDOUT_FILENO);
            fclose(fd_out);
        }

        if (debug) {
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
            fprintf(stderr, "(child2>going to execute cmd: %s)\n", pCmdLine->next->arguments[0]);
        }

        execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments); //execute right-hand command
        perror("execvp failed");
        _exit(EXIT_FAILURE);
    }

    //parent process
    close(pipefd[0]);  //close the read end of the pipe in the parent
    close(pipefd[1]);  //close the write end of the pipe in the parent

    if (debug) {
        fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
        fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    }

    waitpid(cpid1, NULL, 0);  //wait for the first child to terminate
    waitpid(cpid2, NULL, 0);  //wait for the second child to terminate

    if (debug) fprintf(stderr, "(parent_process>exiting…)\n");
}

void execute(cmdLine *pCmdLine,const char* input) {
    if (!strcmp("!!", pCmdLine->arguments[0])){
        if (history_tail == NULL)
        {
            return;
        }
        printf("%s", history_tail->cmd);
        input = history_tail->cmd;
        pCmdLine = parseCmdLines(history_tail->cmd);

    }
    else if (!strncmp("!", pCmdLine->arguments[0],1)){
        if (history_tail == NULL)
        {
            return;
        }
        input = history_tail->cmd;

        if( print_history_node_on_index(atoi(pCmdLine->arguments[0]+1))!=NULL) {
            pCmdLine = parseCmdLines(print_history_node_on_index(atoi(pCmdLine->arguments[0]+1)));
        }
        else{
            return;
        }
    }

    if (strcmp("procs", pCmdLine->arguments[0]) == 0) {
        //handle "procs" command
        updateProcessList(&list);
        printProcessList(&list);
        add_history_node(input);

        freeCmdLines(pCmdLine);

        return;
    }

    if (strcmp(pCmdLine->arguments[0], "cd") == 0) {
        //handle "cd" command
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(pCmdLine->arguments[1]) != 0) {
                perror("cd failed");
            }
        }
        add_history_node(input);

        return;
    }
    if (strcmp(pCmdLine->arguments[0], "sleep") == 0) {
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "sleep: missing argument\n");
        } else {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGTSTP) == 0) {
                printf("Sent SIGTSTP to process %d\n", pid);
                updateProcessStatus(list, pid, SUSPENDED); // Assuming SUSPENDED is the status after SIGTSTP
            } else {
                perror("kill (SIGTSTP) failed");
            }
        }
        add_history_node(input);

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
        add_history_node(input);

        return;
    }

    if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        if (pCmdLine->argCount < 2) {
            fprintf(stderr, "blast: missing argument\n");
        } else {
            pid_t pid = atoi(pCmdLine->arguments[1]);
            if (kill(pid, SIGINT) == 0) {
                printf("Sent SIGINT to process %d\n", pid);
                updateProcessStatus(list, pid, TERMINATED);
            } else {
                perror("kill (SIGINT) failed");
            }
        }
        add_history_node(input);

        return;
    }



    if (!strcmp("history", pCmdLine->arguments[0])){
        print_history_node(history_head);
    }

    else if (pCmdLine->next != NULL) {
        add_history_node(input);
        //execute pipeline commands
        execute_pipe_commands(pCmdLine);
    } else {
        add_history_node(input);
        //execute single command
        execute_single_command(pCmdLine);
    }
}



int main(int argc, char **argv) {
    char input[MAX_INPUT_SIZE];
    char cwd[PATH_MAX];
    cmdLine *parsedLine;

    //check for the -d flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
    }

    //setup signal handler
    signal(SIGINT, handle_sigint);

    while (1) {
        //display prompt
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
        } else {
            perror("getcwd() error");
            return 1;
        }

        //read input
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            perror("fgets failed");
            continue;
        }

        //remove newline character from input
        input[strcspn(input, "\n")] = '\0';

        //parse input
        parsedLine = parseCmdLines(input);
        if (parsedLine == NULL) {
            continue;
        }

        //check for "quit" command
        if (strcmp(parsedLine->arguments[0], "quit") == 0) {
            freeCmdLines(parsedLine);
            break;
        }

        //execute the command
        execute(parsedLine,input);


    }
    freeProcessList(list);

    return 0;
}
