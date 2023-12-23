#include <math.h>

// function to create an array of log structs
struct log *createLogArray(int size)
{
    struct log *logArray = (struct log *)malloc(size * sizeof(struct log));
    return logArray;
}

// function to create a log struct
struct log createLog(int id, int currTime, int state, int arrivalTime, int runTime, int remainingTime, int waitingTime)
{
    int turnAroundTime = currTime - arrivalTime;
    // weighted turnaround to the nearest 2 decimal places
    float weightedTurnAroundTime = (float)turnAroundTime / runTime;
    struct log Log = {id, currTime, state, arrivalTime, runTime, remainingTime,
                      waitingTime, turnAroundTime, weightedTurnAroundTime};
    return Log;
}

// function to add a log to the log array
void addLog(struct log **logArray, int *size, struct log Log)
{
    // reallocating the array to add the new log
    *logArray = (struct log *)realloc(*logArray, (*size + 1) * sizeof(struct log));
    (*logArray)[*size] = Log;
    (*size)++;
}

void SRTN_checkForProcessCompletion(struct process **runningProcess, struct log **logArray, int *logArraySize, Array* waitingList, PriorityQueue *priorityQueue, memoryLogArray* memoryLogs)
{
    int status;
    // Check for process completion
    if (*runningProcess)
    {
        if (waitpid((*runningProcess)->pid, &status, WNOHANG) > 0)
        {
            // Child process finished
            // printf("Process %d is completed at time %d\n", (*runningProcess)->id, getClk());
            // Create a finished log
            struct log Log = createLog((*runningProcess)->id, getClk(), 3, (*runningProcess)->arrival, (*runningProcess)->runtime, 0, (*runningProcess)->waitTime);
            addLog(logArray, logArraySize, Log);
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

bool SRTN_DetectAndHandlePreemption(PriorityQueue *priorityQueue, struct process **runningProcess, struct log **logArray, int *logArraySize)
{
    if (*runningProcess && !isEmpty(priorityQueue))
    {
        struct process *minProcess = heapMinimum(priorityQueue);
        if (minProcess->remainingTime < (*runningProcess)->remainingTime)
        {
            // Create a preemption log for stopped process
            struct log Log = createLog((*runningProcess)->id, getClk(), 1, (*runningProcess)->arrival, (*runningProcess)->runtime, (*runningProcess)->remainingTime, (*runningProcess)->waitTime);
            addLog(logArray, logArraySize, Log);
            // store stop time
            (*runningProcess)->stopTime = getClk();

            (*runningProcess)->priority = (*runningProcess)->remainingTime;
            minHeapInsert(priorityQueue, *runningProcess);
            *runningProcess = NULL;
            return true;
        }
    }

    return false;
}

void SRTN_receiveProcess(struct msgbuff message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag, Array* waitingList, memoryLogArray* memoryLogs)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
    {
        *allProcessesSentFlag = true; // All processes have been received
        // printf("Received Termination Message at time %d\n", getClk());
    }
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));

        // The process's priority, which is the last input to the function, is set as the runtime; since we are using SRTN.
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.runtime, message.p.memorySize);

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

        // printf("Received Message %d at time %d\n", message.p.id, getClk());
    }
}

void SRTN_receiveProcesses(int msgq_id, struct msgbuff *message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag, Array* waitingList, memoryLogArray* memoryLogs)
{
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        SRTN_receiveProcess(*message, priorityQueue, allProcessesSentFlag, waitingList, memoryLogs);
    }
}

void SRTN(int sch_child_msgq_id)
{
    // Initialize message queue
    printf("SRTN: Starting Algorthim...\n\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");

    // Initialise log array
    int logArraySize = 0;
    struct log *logArray = createLogArray(logArraySize);
    memoryLogArray* memoryLogs = (memoryLogArray*) malloc(sizeof(memoryLogArray));
    initMemoryLogArray(memoryLogs, 1);

    // Initialize idle counter and total time
    int idleCounter = 0;
    int totalTime = 0;

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
            SRTN_checkForProcessCompletion(&runningProcess, &logArray, &logArraySize, waitingList, priorityQueue, memoryLogs);
            SRTN_receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag, waitingList, memoryLogs);
        } else {
            // Check for process completion
            SRTN_checkForProcessCompletion(&runningProcess, &logArray, &logArraySize, waitingList, priorityQueue, memoryLogs);
        }

        // Check for any preemptions
        preemptionFlag = SRTN_DetectAndHandlePreemption(priorityQueue, &runningProcess, &logArray, &logArraySize);
        // printf("Preemption Flag: %d\n", preemptionFlag);
        //  Check if the CPU is idle and there is a process to run
        if ((!runningProcess && !isEmpty(priorityQueue)))
        {
            runningProcess = heapExtractMin(priorityQueue);
            // printf("Remaining time: %d Run time: %d\n", runningProcess->remainingTime, runningProcess->runtime);
            if (runningProcess->remainingTime < runningProcess->runtime)
            {
                // Store resume time
                runningProcess->resumeTime = getClk();
                runningProcess->waitTime += runningProcess->resumeTime - runningProcess->stopTime;
                struct log Log = createLog(runningProcess->id, getClk(), 2, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->waitTime);
                addLog(&logArray, &logArraySize, Log);
            }
            if (runningProcess->pid == -1)
            {
                runningProcess->pid = fork();
                runningProcess->waitTime += getClk() - runningProcess->arrival;
                struct log Log = createLog(runningProcess->id, getClk(), 0, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->waitTime);
                // Add the process to the log array
                addLog(&logArray, &logArraySize, Log);

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
            runningProcess->remainingTime--;
            // printf("Remaining time: %d\n", runningProcess->remainingTime);
        }
    }
    // idleCounter - 2 if you want to exclude last loop where it makes sure all procs ended
    idleCounter = idleCounter - 2;
    printf("SRTN: CPU is idle for %d seconds\n", idleCounter);
    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    msgctl(sch_child_msgq_id, IPC_RMID, NULL);
    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
    printf("SRTN: Finished Algorthim...\n");

    // Store the log file
    storePerfAndLogFiles(logArray, logArraySize, idleCounter, memoryLogs);
}