#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) 
{
    pid_t childPids[2];
    int fd[2];
    int status;
    int child;
    char buff[1024];

    char msgs[2][100] = {"aaaaa\n", "bbbbbbbbbbbbbbbbb\n"};

    printf("parent pid: %d, group: %d\n", getpid(), getpgrp());
    if (pipe(fd) == -1) 
    {
        perror("cant pipe\n");
        exit(1);
    }

    for (int i = 0; i < 2; i++) {
        if ((childPids[i]=fork()) == -1) {
            perror("Can`t fork\n");
            exit(1);
        } else if (childPids[i] == 0) {
            printf("Child: pid = %d, ppid = %d, grid = %d\n", getpid(), getppid(), getpgrp());
            close(fd[0]);
            write(fd[1], msgs[i], sizeof(msgs[i]));
            printf("%d sent message: %s\n", getpid(), msgs[i]);
            exit(0);
        } else {
            pid_t waited_child = waitpid(childPids[i], &status, WCONTINUED);
            if (waited_child == -1) {
                printf("Can't wait\n");
                return 1;
            }
            printf("childpid = %d, parentpid = %d, status = %d\n", waited_child, getpid(), status); 
            if (WIFEXITED(status)) {
                printf("exited, status=%d\n\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d\n\n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d\n\n", WSTOPSIG(status));
            }
            read(fd[0], buff, sizeof(msgs[i]));
            printf("%d read message: %s\n", getpid(), buff);
            memset(buff, 0, sizeof(buff));
        }
    }

    close(fd[1]);
    read(fd[0], buff, sizeof(buff));
    printf("%d read message: %s\n", getpid(), buff);

    return 0;
}

