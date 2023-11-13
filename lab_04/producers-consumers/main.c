#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <string.h>

#define N 64

#define CONSUMERS_AMOUNT 3
#define PRODUCERS_AMOUNT 3

#define SB 0
#define SE 1
#define SF 2

#define P -1
#define V  1

static int flag = 1;

char *prod;
char *cons;
char *current_char;

struct sembuf start_produce[2] = {
    {SE, P, 0}, 
    {SB, P, 0},
};
struct sembuf stop_produce[2] =  { 
    {SB, V, 0}, 
    {SF, V, 0},    
};
struct sembuf start_consume[2] = { 
    {SF, P, 0}, 
    {SB, P, 0},
};
struct sembuf stop_consume[2] =  { 
    {SB, V, 0}, 
    {SE, V, 0},
};

void sig_handler(int sig_numb)
{
    flag = 0;
    printf("\nSignal %d catch by pid=%d.\n", sig_numb, getpid());
}

void write_b(char **addr, char symb)
{
    prod = *addr;
    *prod = symb;
    *addr = ++prod; 	
}

void read_b(char **addr, char *dst)
{
    cons = *(addr + 1);
    *dst = *cons;
    *(addr + 1) = ++cons; 
}

void consumer_run(char* addr, const int sem_id, const int cons_id)
{
    srand(getpid());
    while (flag) {
        sleep(rand() % 2);
        
        char ch;
        int rc = semop(sem_id, start_consume, 2);

        if (rc == -1)
        {
            perror("Consumer enter semop error");
            exit(1);
        }

        read_b((char**)addr, &ch);
        ch = (ch - 97) % 26 + 97;

        printf(" \e[1;33mConsumer (pid=%d) #%d: get '%c'\e[0m\n", getpid(), cons_id, ch);
        
        rc = semop(sem_id, stop_consume, 2);

        if (rc == -1)
        {
            perror("Consumer exit semop error");
            exit(1);
        }
    }
}

void producer_run(char* addr, int sem_id, int prod_id)
{
    srand(getpid());
    while (flag) {
        sleep(rand() % 3);

        int rc = semop(sem_id, start_produce, 2);

        if (rc == -1)
        {
            perror("Producer enter semop error");
            exit(1);
        }

        char s = (*current_char)++;
        s = (s - 97) % 26 + 97;
        write_b((char**)addr, s);

        printf(" \e[1;32mProducer (pid=%d) #%d: put '%c'\e[0m \n", getpid(), prod_id, s);
        
        rc = semop(sem_id, stop_produce, 2);

        if (rc == -1)
        {
            perror("Producer exit semop error");
            exit(1);
        }
    }
}

int main(void)
{
    signal(SIGINT, sig_handler);
    printf("Press Ctrl-C to raise a signal\n");

    setbuf(stdout, NULL);
    srand(time(NULL));
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    key_t key = ftok("./file", 1);
    
    if (key == -1)
    {
        perror("ftok error");
        exit(1);
    }

    int shmid = shmget(key, 256, IPC_CREAT | perms); 
    
    if (shmid == -1)
    {
        perror("shmget error");
        exit(1);
    }

    char *addr = (char *)shmat(shmid, NULL, 0);

    if (addr == (char*)-1)
    {
        perror("shmat error");
        exit(1);
    }

    current_char = addr + 80;
    char **tmp = (char**) addr;
    *tmp = *(tmp + 1) = addr + (2 * sizeof(char*));
    *current_char = 'a';

    int sem_fd =  semget(key, 3, IPC_CREAT | perms);

    if (sem_fd == -1)
    {
        perror("semget error");
        exit(1);
    }
    
    if (semctl(sem_fd, SB, SETVAL, 1) == -1)
    {
        perror("semctl error 2");
        exit(1);
    }
    if (semctl(sem_fd, SE, SETVAL, N) == -1) 
    {
        perror("semctl error 1");
        exit(1);
    }
    if (semctl(sem_fd, SF, SETVAL, 0) == -1) 
    {
        perror("semctl error 0");
        exit(1);
    }

    for (int i = 0; i < PRODUCERS_AMOUNT; i++) {
        pid_t childpid;

        if ((childpid = fork()) == -1)
        {
            perror("Producer fork error");
            exit(1);
        }
        else if (childpid == 0)
        {
            producer_run(addr, sem_fd, i + 1);
            exit(0);
        }
    }

    for (int i = 0; i < CONSUMERS_AMOUNT; i++) {
        pid_t childpid;

        if ((childpid = fork()) == -1)
        {
            perror("Consumer for error");
            exit(1);
        }
        else if (!childpid)
        {
            consumer_run(addr, sem_fd, i + 1);
            exit(0);
        }
    }

    for (size_t i = 0; i < PRODUCERS_AMOUNT + CONSUMERS_AMOUNT; i++)
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

    if (shmctl(shmid, IPC_RMID, NULL) ||  (shmdt((void *)addr) == -1) || (semctl(sem_fd, 3, IPC_RMID) == -1))
    {
        perror("error");
        exit(1);
    }
    
    return 0;
}
