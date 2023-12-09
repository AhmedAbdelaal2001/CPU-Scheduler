#include "priority_queue.h"

int sch_child_sem_id;

void receiveProcess(struct msgbuff message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
        *allProcessesSentFlag = true; // All processes have been received
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority);
        // Insert the process into the priority queue
        minHeapInsert(priorityQueue, newProcess);
    }
}

void receiveProcesses(int msgq_id, struct msgbuff *message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
{
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
        receiveProcess(*message, priorityQueue, allProcessesSentFlag);
}

void HPF()
{
    // Initialize message queue
    printf("HPF: Starting Algorthim...\n");
    int msgq_id = prepareMessageQueue();
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");
    sch_child_sem_id = prepareSemaphore("keys/sch_child_sem_key", 0);

    struct msgbuff message;

    // Initialize priority queue
    PriorityQueue *priorityQueue = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    initializePriorityQueue(priorityQueue);

    bool allProcessesSentFlag = false;
    bool processRunningFlag = false;

    int status;
    pid_t child_pid = -1; // Track the PID of the running child process

    while (!isEmpty(priorityQueue) || !allProcessesSentFlag || processRunningFlag)
    {
        // printf("HPF: Looping...\n");
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            // printf("Downing gen_sch_sem_id\n");
        }

        // Receive a process from the message queue
        if (!allProcessesSentFlag)
            receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag);

        // Check if the CPU is idle and there is a process to run
        if (!processRunningFlag && !isEmpty(priorityQueue))
        {
            struct process *nextProcess = heapExtractMin(priorityQueue);
            child_pid = fork();
            if (child_pid == 0)
            {
                int currTime = getClk(), iter = 0;

                while (nextProcess->remainingTime != 0)
                {
                    // printf("Downing sch_child_sem_id at time %d\n", getClk());
                    down(sch_child_sem_id);
                    printf("Process %d running at time %d\n", nextProcess->id, getClk());
                    nextProcess->remainingTime--;
                    // iter++;
                    // if (nextProcess->remainingTime != 0)
                }

                // printf("Upping sch_child_sem_id outside loop\n");
                // up(sch_child_sem_id);
                free(nextProcess);
                exit(0);
            }
            else if (child_pid > 0)
            {
                // Update CPU state to idle
                processRunningFlag = true;
                // up(sch_child_sem_id);
                // printf("Upping sch_child_sem_id at time %d\n", currTime);
            }
            else
                // Handle fork error
                perror("Fork failed");
        }

        // Check for process completion
        checkForProcessCompletion(&processRunningFlag, &child_pid);

        if (processRunningFlag)
        {
            int currTime = getClk();
            up(sch_child_sem_id);
            // printf("Upping sch_child_sem_id at time %d\n", currTime);

            while (currTime == getClk())
            {
            }
        }
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);

    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
}
