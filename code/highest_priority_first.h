#include "priority_queue.h"

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

    doneSending = false;
}

void HPF()
{
    // Initialize message queue
    printf("HPF: Starting Algorthim...\n");
    int msgq_id = prepareMessageQueue();
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
        // Check for process completion
        checkForProcessCompletion(&processRunningFlag, &child_pid);

        // Receive a process from the message queue
        if (!allProcessesSentFlag && doneSending)
            receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag);

        // Check if the CPU is idle and there is a process to run
        if (!processRunningFlag && !isEmpty(priorityQueue))
        {
            struct process *nextProcess = heapExtractMin(priorityQueue);
            child_pid = fork();
            if (child_pid == 0)
            {
                // Child process: simulate process execution
                int prevTime = getClk();
                printf("Process %d running at time %d\n", nextProcess->id, getClk());

                while (nextProcess->remainingTime != 0)
                {
                    if (getClk() != prevTime)
                    {
                        if (nextProcess->remainingTime != 1)
                            printf("Process %d running at time %d\n", nextProcess->id, getClk());
                        prevTime = getClk();
                        nextProcess->remainingTime--;
                    }
                }
                exit(0);
            }
            else if (child_pid > 0)
                // Update CPU state to idle
                processRunningFlag = true;
            else
                // Handle fork error
                perror("Fork failed");
        }
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);

    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
}
