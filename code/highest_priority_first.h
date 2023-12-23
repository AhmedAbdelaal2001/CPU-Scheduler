#include <math.h>

// function to create an array of log structs
struct log *HPF_createLogArray(int size)
{
    struct log *logArray = (struct log *)malloc(size * sizeof(struct log));
    return logArray;
}

// function to create a log struct
struct log HPF_createLog(int id, int currTime, int state, int arrivalTime, int runTime, int remainingTime, int waitingTime)
{
    int turnAroundTime = currTime - arrivalTime;
    // weighted turnaround to the nearest 2 decimal places
    float weightedTurnAroundTime = (float)turnAroundTime / runTime;
    struct log Log = {id, currTime, state, arrivalTime, runTime, remainingTime,
                      waitingTime, turnAroundTime, weightedTurnAroundTime};
    return Log;
}

// function to add a log to the log array
void HPF_addLog(struct log **logArray, int *size, struct log Log)
{
    // reallocating the array to add the new log
    *logArray = (struct log *)realloc(*logArray, (*size + 1) * sizeof(struct log));
    (*logArray)[*size] = Log;
    (*size)++;
}

void HPF_checkForProcessCompletion(struct process **runningProcess, struct log **logArray, int *logArraySize, Array* waitingList, PriorityQueue *priorityQueue, memoryLogArray* memoryLogs)
{
    int status;
    // Check for process completion
    if (*runningProcess)
    {
        if (waitpid((*runningProcess)->pid, &status, WNOHANG) > 0)
        {
            // Child process finished
            struct log Log = HPF_createLog((*runningProcess)->id, getClk(), 3, (*runningProcess)->arrival, (*runningProcess)->runtime, 0, (*runningProcess)->waitTime);
            HPF_addLog(logArray, logArraySize, Log);
            memoryLog* newMemoryLog = (memoryLog*)malloc(sizeof(memoryLog));
            setMemoryLog(newMemoryLog, (*runningProcess)->id, getClk(), (*runningProcess)->memorySize, (*runningProcess)->assignedBlock->memoryLocation, (*runningProcess)->assignedBlock->memoryLocation + (*runningProcess)->assignedBlock->size - 1, 0);
            addElement_memoryLogArray(memoryLogs, newMemoryLog);
            //printf("At time %d freed %d bytes from process %d from %d to %d\n", getClk(), (*runningProcess)->memorySize, (*runningProcess)->id, (*runningProcess)->assignedBlock->memoryLocation, (*runningProcess)->assignedBlock->memoryLocation + (*runningProcess)->assignedBlock->size - 1);
            
            deallocate((*runningProcess)->assignedBlock);
            free(*runningProcess);
            *runningProcess = NULL;

            while (waitingList->size > 0) {
                struct process* currProcess = waitingList->data[0];
                Block* allocatedBlock = allocate(currProcess->memorySize);
                if (allocatedBlock) {
                    memoryLog* newMemoryLog = (memoryLog*)malloc(sizeof(memoryLog));
                    setMemoryLog(newMemoryLog, currProcess->id, getClk(), currProcess->memorySize, allocatedBlock->memoryLocation, allocatedBlock->memoryLocation + allocatedBlock->size - 1, 1);
                    addElement_memoryLogArray(memoryLogs, newMemoryLog);
                    //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), currProcess->memorySize, currProcess->id, allocatedBlock->memoryLocation, allocatedBlock->memoryLocation + allocatedBlock->size - 1);
                    
                    currProcess->assignedBlock = allocatedBlock;
                    removeElement(waitingList, 0);
                    //printf("Removed Element from waiting list\n");
                    minHeapInsert(priorityQueue, currProcess);
                } else {
                    //printf("Could Not Remove Element from waiting list\n");
                    break;
                }
            }
        }
    }
}

void HPF_receiveProcess(struct msgbuff message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag, Array* waitingList, memoryLogArray* memoryLogs)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
    {
        *allProcessesSentFlag = true; // All processes have been received
    }
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));

        // The process's priority, which is the last input to the function, is set as the runtime; since we are using SRTN.
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority, message.p.memorySize);

        // Insert the process into the priority queue. Since the priority of the process is set to its runtime,
        // the priority queue will automatically handle the ordering of the values with respect to the running time.
        Block* allocatedBlock = allocate(newProcess->memorySize);
        if (allocatedBlock) {
            memoryLog* newMemoryLog = (memoryLog*)malloc(sizeof(memoryLog));
            setMemoryLog(newMemoryLog, newProcess->id, getClk(), newProcess->memorySize, allocatedBlock->memoryLocation, allocatedBlock->memoryLocation + allocatedBlock->size - 1, 1);
            addElement_memoryLogArray(memoryLogs, newMemoryLog);
            //printf("At time %d allocated %d bytes for process %d from %d to %d\n", getClk(), newProcess->memorySize, newProcess->id, allocatedBlock->memoryLocation, allocatedBlock->memoryLocation + allocatedBlock->size - 1);
            
            newProcess->assignedBlock = allocatedBlock;
            minHeapInsert(priorityQueue, newProcess);
        } else {
            addElement(waitingList, newProcess);
            //printf("Added Element to Waiting List\n");
        }
    }
}

void HPF_receiveProcesses(int msgq_id, struct msgbuff *message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag, Array* waitingList, memoryLogArray* memoryLogs)
{
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        HPF_receiveProcess(*message, priorityQueue, allProcessesSentFlag, waitingList, memoryLogs);
    }
}

void HPF(int sch_child_msgq_id)
{
    // Initialize message queue
    printf("HPF: Starting Algorthim...\n\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");

    // Initialise log array
    int logArraySize = 0;
    struct log *logArray = HPF_createLogArray(logArraySize);
    memoryLogArray* memoryLogs = (memoryLogArray*) malloc(sizeof(memoryLogArray));
    initMemoryLogArray(memoryLogs, 1);

    // Initialize idle counter and total time
    int idleCounter = 0;

    struct msgbuff message;
    struct msgbuff sch_child_message;
    // Initialize priority queue
    PriorityQueue *priorityQueue = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    initializePriorityQueue(priorityQueue);

    // Initialize Waiting List
    Array *waitingList = (Array *)malloc(sizeof(Array));
    initArray(waitingList, 1);

    bool allProcessesSentFlag = false;
    bool preemptionFlag = false;
    struct process *runningProcess = NULL;

    int status;
    pid_t child_pid = -1; // Track the PID of the running child process

    while (!isEmpty(priorityQueue) || !isArrEmpty(waitingList) || !allProcessesSentFlag || runningProcess)
    {
        // Receive a process from the message queue
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            HPF_checkForProcessCompletion(&runningProcess, &logArray, &logArraySize, waitingList, priorityQueue, memoryLogs);
            HPF_receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag, waitingList, memoryLogs);
        } else {
            // Check for process completion
            HPF_checkForProcessCompletion(&runningProcess, &logArray, &logArraySize, waitingList, priorityQueue, memoryLogs);
        }

        // Check if the CPU is idle and there is a process to run
        if ((!runningProcess && !isEmpty(priorityQueue)))
        {
            runningProcess = heapExtractMin(priorityQueue);
            if (runningProcess->pid == -1)
            {
                runningProcess->pid = fork();
                runningProcess->waitTime += getClk() - runningProcess->arrival;
                struct log Log = HPF_createLog(runningProcess->id, getClk(), 0, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->waitTime);
                // Add the process to the log array
                HPF_addLog(&logArray, &logArraySize, Log);

                if (runningProcess->pid == -1)
                    perror("Fork Falied");
                else if (runningProcess->pid == 0)
                    runProcess(runningProcess);
            }
        }
        else if (!runningProcess && isEmpty(priorityQueue))
        {
            // printf("Incrementing idle counter\n");
            idleCounter++;
        }

        if (runningProcess)
        {
            sch_child_message.mtype = runningProcess->pid;
            msgsnd(sch_child_msgq_id, &sch_child_message, sizeof(sch_child_message.p), !IPC_NOWAIT);
            if (allProcessesSentFlag)
            {
                int currTime = getClk();
                while (currTime == getClk())
                {
                }
            }
        }
    }
    // idleCounter - 2 if you want to exclude last loop where it makes sure all procs ended
    idleCounter = idleCounter - 2;
    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    msgctl(sch_child_msgq_id, IPC_RMID, NULL);
    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
    storePerfAndLogFiles(logArray, logArraySize, idleCounter, memoryLogs);
}
