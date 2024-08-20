#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t cpid1, cpid2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    //create the first child process
    if ((cpid1 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid1 == 0) {    //child 1: ls -l
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        close(STDOUT_FILENO);               //close standard output
        dup(pipefd[1]);                     //duplicate the write end of the pipe to standard output
        close(pipefd[1]);                   //close the original write end of the pipe
        close(pipefd[0]);                   //close the read end of the pipe
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
        execlp("ls", "ls", "-l", (char *) NULL);   //execute "ls -l"
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    //parent process
    fprintf(stderr, "(parent_process>created process with id: %d)\n", cpid1);
    fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
    close(pipefd[1]);  // close the write end of the pipe in the parent

    fprintf(stderr, "(parent_process>forking...)\n");
    //create the second child process
    if ((cpid2 = fork()) == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid2 == 0) {    // child 2: tail -n 2
        fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
        close(STDIN_FILENO);                //close standard input
        dup(pipefd[0]);                     //duplicate the read end of the pipe to standard input
        close(pipefd[0]);                   //close the original read end of the pipe
        fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
        execlp("tail", "tail", "-n", "2", (char *) NULL);  //execute "tail -n 2"
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    //parent process
    fprintf(stderr, "(parent_process>created process with id: %d)\n", cpid2);
    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
    close(pipefd[0]);  //close the read end of the pipe in the parent

    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
    waitpid(cpid1, NULL, 0);  //wait for the first child to terminate
    waitpid(cpid2, NULL, 0);  //wait for the second child to terminate

    fprintf(stderr, "(parent_process>exiting…)\n");

    return 0;
}
