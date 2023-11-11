#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t childPids[2];

    char *path1 = "./app3-1.exe";
    char *path2 = "./app3-2.exe";

    char *paths[2] = {path1, path2};

    for (int i = 0; i < 2; i++)  {
        if ((childPids[i] = fork()) == -1) {
            printf("Cant't fork\n");
            exit(1);
        } else if (childPids[i] == 0) {
            printf("Child: pid = %d, ppid = %d, grid = %d\n", getpid(), getppid(), getpgrp());
            if (execv(paths[i], NULL) == -1) {
                printf("Can't execute file!");
                exit(1);
            }
        } else {
            printf("Parent: pid = %d, grid = %d\n", getpid(), getpgrp());
        }
    }

    int status;
    pid_t childPid;
    for (int i = 0; i < 2; i++) {
        childPid =  waitpid(childPids[i], &status, WUNTRACED | WCONTINUED);
        if (childPid == -1) {
            printf("Can't wait\n");
            exit(1);
        }
        printf("Child #%ld has finished: PID = %d\n", i, childPid);

        if (WIFEXITED(status)) {
            printf("exited, status=%d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d\n", WSTOPSIG(status));
        }
    }

    return 0;
}
