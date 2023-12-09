#include "priority_queue.h"

void checkForProcessCompletion(struct process** runningProcess)
{
    int status;
    // Check for process completion
    if (*runningProcess)
    {
        if (waitpid((*runningProcess)->pid, &status, WNOHANG) > 0)
        {
            // Child process finished
            *runningProcess = NULL;
        }
    }
}

bool checkForPreemption(PriorityQueue *priorityQueue, struct process** runningProcess) {
    if (*runningProcess && !isEmpty(priorityQueue)) {
        struct process* minProcess = heapMinimum(priorityQueue);
        if (minProcess->remainingTime < (*runningProcess)->remainingTime) {
            (*runningProcess)->priority = (*runningProcess)->remainingTime;
            minHeapInsert(priorityQueue, *runningProcess);
            return true;
        }
    }

    return false;
}

struct process* handlePreemption(struct process* runningProcess) {
    // This function should stop the currently running process, then return NULL.

    //TODO: Preemption Logic...

    //Return NULL
    return NULL;
}

void receiveProcess(struct msgbuff message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
        *allProcessesSentFlag = true; // All processes have been received
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));
        
        // The process's priority, which is the last input to the function, is set as the runtime; since we are using SRTN.
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.runtime);
        
        // Insert the process into the priority queue. Since the priority of the process is set to its runtime,
        // the priority queue will automatically handle the ordering of the values with respect to the running time.
        minHeapInsert(priorityQueue, newProcess);
    }
}

void receiveProcesses(int msgq_id, struct msgbuff *message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
{
    int currTime = getClk();
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        receiveProcess(*message, priorityQueue, allProcessesSentFlag);
    }
}

void childProcessCode(struct process* runningProcess, int sch_child_sem_id) {
    while (runningProcess->remainingTime != 0)
    {
        // printf("Downing sch_child_sem_id at time %d\n", getClk());
        down(sch_child_sem_id);

        printf("Process %d running at time %d\n", runningProcess->id, getClk());
        runningProcess->remainingTime--;
    }
    free(runningProcess);
    exit(0);
}

void SRTN()
{
    // Initialize message queue
    printf("SRTN: Starting Algorthim...\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");
    int sch_child_sem_id = prepareSemaphore("keys/sch_child_sem_key", 0);

    struct msgbuff message;

    // Initialize priority queue
    PriorityQueue *priorityQueue = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    initializePriorityQueue(priorityQueue);

    bool allProcessesSentFlag = false;
    bool preemptionFlag = false;
    struct process* runningProcess = NULL;

    int status;
    pid_t child_pid = -1; // Track the PID of the running child process

    while (!isEmpty(priorityQueue) || !allProcessesSentFlag || runningProcess)
    {
        // Receive a process from the message queue
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag);
        }

        // Check for process completion
        checkForProcessCompletion(&runningProcess);

        // Check for any preemptions
        if (checkForPreemption(priorityQueue, &runningProcess))
            runningProcess = handlePreemption(runningProcess);
        
        // Check if the CPU is idle and there is a process to run
        if ((!runningProcess && !isEmpty(priorityQueue)))
        {
            runningProcess = heapExtractMin(priorityQueue);
            if (runningProcess->pid == -1) {
                runningProcess->pid = fork();
                if (runningProcess->pid == -1) perror("Fork Falied");
                else if (runningProcess->pid == 0) childProcessCode(runningProcess, sch_child_sem_id);
            } else {
                // TODO:
                //runningProcess was previously stopped, continue it from where it left.
            }
        }

        if (runningProcess)
        {
            up(sch_child_sem_id);
            int currTime = getClk();
            while (currTime == getClk()) {}
        }
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);

    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
}
