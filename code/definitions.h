// Define a constant for the maximum number of processes
#define MAX_PROCESSES 100
#define MAX_MEM_SIZE 1024  // Assuming a total memory size of 1024 bytes

// A special message type to indicate that all processes have been sent
#define TERMINATION_MSG_TYPE 999

#define true 1
#define false 0
typedef short bool;

// our stuff

// Blocks will be stored as a Doubly Linked List. This struct defines the structure of each block.
// The Linked List will only store the nodes at the leaves of the tree; no further information is required to complete the algorithm.
typedef struct Block {
    int size; // The maximum capacity of the block in Bytes
    int index; // The index of the block if the nearly complete binary tree was stored in an array, and indexed in the same way as a binary heap
    int isFree; // Indicates whether the block is occupied or not
    int memoryLocation; // Indicates the first memory location occupied by the block
    struct Block* next; // Pointer to the next block
    struct Block* prev; // Pointer to the previous block
} Block;

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
    int memorySize;
    Block* assignedBlock;

    int remainingTime;
    int pid;
    int stopTime;
    int waitTime;
    int resumeTime;
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
struct process create_process(int id, int arrival, int runtime, int priority, int memorySize)
{
    struct process p;
    p.id = id;
    p.arrival = arrival;
    p.runtime = runtime;
    p.priority = priority;
    p.memorySize = memorySize;
    p.assignedBlock = NULL;

    p.remainingTime = runtime;
    p.pid = -1;

    return p;
}

void setProcessInformation(struct process *newProcess, int id, int arrival, int runtime, int priority, int memorySize)
{
    newProcess->id = id;
    newProcess->arrival = arrival;
    newProcess->runtime = runtime;
    newProcess->priority = priority;
    newProcess->memorySize = memorySize;
    newProcess->assignedBlock = NULL;
    
    newProcess->waitTime = 0;
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
}

void runProcess(struct process *runningProcess)
{
    // convert id, remaining time to send through execl
    char id[10];
    char remainingTime[10];
    sprintf(id, "%d", runningProcess->id);
    sprintf(remainingTime, "%d", runningProcess->remainingTime);
    execl("process.out", "process.out", id, remainingTime, NULL);
}

// Log file
struct log
{
    int id;
    int currTime;
    int state; // 0: started, 1: stopped 2: resumed 3: finished
    int arrivalTime;
    int runTime;
    int remainingTime;
    int waitTime;
    int turnAroundTime;           // finish time - arrival time
    float weightedTurnAroundTime; // turnAroundTime / runTime
};

typedef struct
{
    int id;
    int currTime;
    int processSize;
    int blockStartingAddress;
    int blockEndingAddress;
    int allocated;
} memoryLog;

typedef struct
{
    memoryLog** data; // Pointer to dynamically allocated array
    int size;              // Current size of the array
    int capacity;          // Maximum capacity of the array
} memoryLogArray;

void setMemoryLog(memoryLog *newMemoryLog, int id, int currTime, int processSize, int blockStartingAddress, int blockEndingAddress, int allocated)
{
    newMemoryLog->id = id;
    newMemoryLog->currTime = currTime;
    newMemoryLog->processSize = processSize;
    newMemoryLog->blockStartingAddress = blockStartingAddress;
    newMemoryLog->blockEndingAddress = blockEndingAddress;
    newMemoryLog->allocated = allocated;
}

int prepareSharedMemory(char *filePath, int size)
{
    key_t key_id = ftok(filePath, 'S'); // use unique key
    int shm_id = shmget(key_id, size, IPC_CREAT | 0666);
    if (shm_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    return shm_id;
}

void storePerfAndLogFiles(struct log *logArray, int logArraySize, int idleCounter, memoryLogArray* memoryLogs)
{
    // Initialize average waiting time and average weighted turnaround time
    float avgWaitingTime = 0;
    float avgWeightedTurnaroundTime = 0;
    // Initialize array of weighted turnaround times
    float *weightedTurnAroundTimes = (float *)malloc(logArraySize * sizeof(float));

    int countTurnAround = 0;
    int countWaiting = 0;

    int totalTime = 0;
    // Print the log array
    // Initialize the log file
    FILE *logFile = fopen("scheduler.log", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    for (int i = 0; i < logArraySize; i++)
    {
        // state = 0 for started, 1 for stopped, 2 for resumed, 3 for finished
        if (logArray[i].state == 3) // process is finished. Need Turnaround time and weighted turnaround time
        {
            // Calculate average waiting time
            countWaiting++;
            avgWaitingTime += logArray[i].waitTime;

            // Calculate turnaround time and weighted turnaround time to the nearest 2 decimal places
            logArray[i].turnAroundTime = logArray[i].currTime - logArray[i].arrivalTime;
            logArray[i].weightedTurnAroundTime = (float)logArray[i].turnAroundTime / logArray[i].runTime;

            // Add the weighted turnaround time to the array
            weightedTurnAroundTimes[countTurnAround] = logArray[i].weightedTurnAroundTime;
            countTurnAround++;
            avgWeightedTurnaroundTime += logArray[i].weightedTurnAroundTime;

            // Print the log to the file
            fprintf(logFile, "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\tTA\t%d\tWTA\t%.2f\n",
                    logArray[i].currTime, logArray[i].id, logArray[i].state == 0 ? "started" : logArray[i].state == 1 ? "stopped"
                                                                                           : logArray[i].state == 2   ? "resumed"
                                                                                                                      : "finished",
                    logArray[i].arrivalTime, logArray[i].runTime, logArray[i].remainingTime, logArray[i].waitTime, logArray[i].turnAroundTime, logArray[i].weightedTurnAroundTime);
        }
        else
        {
            fprintf(logFile, "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
                    logArray[i].currTime, logArray[i].id, logArray[i].state == 0 ? "started" : logArray[i].state == 1 ? "stopped"
                                                                                           : logArray[i].state == 2   ? "resumed"
                                                                                                                      : "finished",
                    logArray[i].arrivalTime, logArray[i].runTime, logArray[i].remainingTime, logArray[i].waitTime);
        }

        // Getting the total time by the last finsihed process
        if (i == logArraySize - 1)
        {
            totalTime = logArray[i].currTime - 1;
        }
    }
    // Calculate the average waiting time and average weighted turnaround time
    fclose(logFile);
    avgWeightedTurnaroundTime /= countTurnAround;

    if (countWaiting == 0) // special case when no process waits
        avgWaitingTime = 0;
    else
        avgWaitingTime /= countWaiting;

    // Compute the standard deviation
    float standardDeviation = 0;
    for (int i = 0; i < countTurnAround; i++)
    {
        standardDeviation += pow(weightedTurnAroundTimes[i] - avgWeightedTurnaroundTime, 2);
    }

    // Store the performance metrics
    FILE *performanceFile = fopen("scheduler.perf", "w");
    // Add CPU utilization to the file
    fprintf(performanceFile, "CPU utilization = %.2f%%\n", (float)(totalTime - idleCounter) / totalTime * 100);
    // Add average waiting time and average weighted turnaround time to the file
    fprintf(performanceFile, "Average Waiting = %.2f\n", avgWaitingTime);
    fprintf(performanceFile, "Average WTA = %.2f\n", avgWeightedTurnaroundTime);
    fprintf(performanceFile, "Std WTA = %.2f\n", sqrt(standardDeviation / countTurnAround));
    fclose(performanceFile);

    FILE *memoryLogFile = fopen("memory.log", "w");
    fprintf(logFile, "#At\ttime\tx\tallocated\ty\tbytes\tfor\tprocess\tz\tfrom\ti\tto\tj\n");
    for (int i = 0; i < memoryLogs->size; i++) {
        char* event = (memoryLogs->data[i]->allocated == 1) ? "allocated":"freed";
        fprintf(memoryLogFile, "At time %d %s %d bytes from process %d from %d to %d\n", memoryLogs->data[i]->currTime, event, memoryLogs->data[i]->processSize, memoryLogs->data[i]->id, memoryLogs->data[i]->blockStartingAddress, memoryLogs->data[i]->blockEndingAddress);
    }
    fclose(memoryLogFile);
}