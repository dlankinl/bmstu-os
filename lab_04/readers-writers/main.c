#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define READERS_AMOUNT  5
#define WRITERS_AMOUNT  3

#define READER_WAIT_C   0
#define READER_ACTIVE_C 1
#define WRITER_WAIT_C   2
#define WRITER_ACTIVE_B 3
#define BIN_WRITER      4

static int flag = 1;

struct sembuf sem_start_read[] = {
    { READER_WAIT_C,    1,  0 },
    { WRITER_ACTIVE_B,  0,  0 },
    { READER_WAIT_C,    -1, 0 },
    { READER_ACTIVE_C,  1,  0 },
};

struct sembuf sem_stop_read[] = {
    { READER_ACTIVE_C,  -1, 0 },
};

struct sembuf sem_start_write[] = {
    { WRITER_WAIT_C,    1,  0 },
    { READER_ACTIVE_C,  0,  0 },
    { WRITER_ACTIVE_B,  -1, 0 },
    { WRITER_WAIT_C,    -1, 0 },
};

struct sembuf sem_stop_write[] = {
    { WRITER_ACTIVE_B,  1,  0 },
};

void sig_handler(int sig_numb)
{
    flag = 0;
    printf("\nSignal %d catch by pid=%d.\n", sig_numb, getpid());
}

int reader_run(char* addr, const int sem_fd, const int rid)
{
    srand(getpid());
    while (flag)
    {
        sleep(rand() % 2);

        if (semop(sem_fd, sem_start_read, 4) == -1)
            exit(1);

        printf(" \e[1;33mReader (pid=%d) #%d: get %d\e[0m\n", getpid(), rid, *addr);

        if (semop(sem_fd, sem_stop_read, 1) == -1) 
            exit(1);
    }
}

int writer_run(char* addr, const int sem_fd, const int wid)
{
    srand(getpid());
    while (flag) {      
        sleep(rand() % 3);

        if (semop(sem_fd, sem_start_write, 4) == -1)
            exit(1);

        printf(" \e[1;32mWriter (pid=%d) #%d: put %d\e[0m \n", getpid(), wid, ++(*addr));
    
        if (semop(sem_fd, sem_stop_write, 1) == -1)
            exit(1);
    }
}

int main(void)
{
    signal(SIGINT, sig_handler);
    printf("Press Ctrl-C to raise a signal\n");

    setbuf(stdout, NULL);
    srand(time(NULL));
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    key_t key = ftok("file", 3);
	
    if (key == -1)
	{
		perror("ftok error");
		exit(1);
	}

    int shmid = shmget(key, sizeof(int), perms | IPC_CREAT);

    if (shmid == -1)
    {
        perror("shmget error");
        exit(1);
    }

    char *addr = (char *)shmat(shmid, NULL, 0);

    if (addr == (char *)-1)
    {
        perror("shmat");
        exit(1);
    }

    if (key == (key_t)-1)
    {
        perror("ftok (key)\n");
        exit(1);
    }

    int sem_fd = semget(key, 5, perms | IPC_CREAT);
    
    if (sem_fd == -1)
    {
        perror("semget");
        exit(1);
    }

    if (semctl(sem_fd, WRITER_ACTIVE_B, SETVAL, 1) == -1)
    {
        perror("semctl error");
        exit(1);
    }

    for (int i = 0; i < WRITERS_AMOUNT; i++) {
        pid_t childpid;

        if ((childpid = fork()) == -1)
        {
            perror("Writer fork error");
            exit(1);
        }
        else if (childpid == 0)
        {
            writer_run(addr, sem_fd, i + 1);
            exit(0);
        }
    }

    for (int i = 0; i < READERS_AMOUNT; i++) {
        pid_t childpid;

        if ((childpid = fork()) == -1)
        {
            perror("Reader for error");
            exit(1);
        }
        else if (!childpid)
        {
            reader_run(addr, sem_fd, i + 1);
            exit(0);
        }
    }

    for (size_t i = 0; i < READERS_AMOUNT + WRITERS_AMOUNT; i++)
    {
        int status;

        pid_t waited_child = wait(&status);
        printf("childpid = %d, parentpid = %d, status = %d\n", waited_child, getpid(), status);
        if (WIFEXITED(status)) {
            printf("exit, status=%d\n\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("killed by signal %d\n\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("stopped by signal %d\n\n", WSTOPSIG(status));
        }
    }

    if (shmctl(shmid, IPC_RMID, NULL))
    {
        perror("shmctl error");
        exit(1);
    }
    if (shmdt((void *)addr) == -1) {
        perror("shmdt error");
        exit(1);
    }
    if (semctl(sem_fd, 3, IPC_RMID) == -1) {
        perror("semctl error");
        exit(1);
    }

    return 0;
}
