#include "stdlib.h"
#include "stdio.h"

// Define a constant for the maximum number of processes
#define MAX_PROCESSES 100

// A special message type to indicate that all processes have been sent
#define TERMINATION_MSG_TYPE 999

#define true 1
#define false 0
typedef short bool;

// our stuff
union Semun
{
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /* Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};

// common structs
struct process
{
    int id;
    int arrival;
    int runtime;
    int priority;

    int remainingTime;
    int pid;
};

struct msgbuff
{
    long mtype;
    struct process p;
};

typedef struct
{
    struct process *processArray[MAX_PROCESSES];
    int size;
} PriorityQueue;

// Process Constructor
struct process create_process(int id, int arrival, int runtime, int priority)
{
    struct process p;
    p.id = id;
    p.arrival = arrival;
    p.runtime = runtime;
    p.priority = priority;

    p.remainingTime = runtime;
    p.pid = -1;

    printf("Process %d created with pid: %d\n", p.id, p.pid);
    return p;
}

void setProcessInformation(struct process *newProcess, int id, int arrival, int runtime, int priority)
{
    newProcess->id = id;
    newProcess->arrival = arrival;
    newProcess->runtime = runtime;
    newProcess->priority = priority;

    newProcess->remainingTime = runtime;
    newProcess->pid = -1;
}

int prepareMessageQueue(char *filePath)
{

    key_t key_id = ftok(filePath, 'M');             // use unique key
    int msgq_id = msgget(key_id, 0666 | IPC_CREAT); // create message queue and return id

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    return msgq_id;
}

int prepareSemaphore(char *filePath, int initVal)
{

    key_t sem_key = ftok(filePath, 'S'); // use unique key
    if (sem_key == -1)
        perror("ftok error");

    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT); // Create semaphore and return it
    if (sem_id < 0)
        perror("Semaphore Error");

    union Semun semun;
    semun.val = initVal; // Initial value of the semaphore
    if (semctl(sem_id, 0, SETVAL, semun) == -1)
        perror("Error in semctl");

    return sem_id;
}

int getSemaphore(char *filePath)
{
    key_t sem_key = ftok(filePath, 'S'); // use unique key
    if (sem_key == -1)
        perror("ftok error");

    int sem_id = semget(sem_key, 1, 0666 | IPC_CREAT); // Create semaphore and return it
    if (sem_id < 0)
        perror("Semaphore Error");

    return sem_id;
}

void down(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }

    // printf("DOWN\n");
}

void up(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }

    // printf("Finished UP\n");
}
