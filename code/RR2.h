#include "Array.h"

int sch_child_msgq_id;

void addProcessToArray(struct msgbuff message, Array *arr, bool *allProcessesSentFlag)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
        *allProcessesSentFlag = true; // All processes have been received
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority);
        // Insert the process into the priority queue
        addElement(arr, newProcess);
    }
}

void addProcessesToArray(int msgq_id, struct msgbuff *message, Array *arr, bool *allProcessesSentFlag)
{
    int currTime = getClk();
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        addProcessToArray(*message, arr, allProcessesSentFlag);
        // printf("SCH: Received process %d at time %d\n", message->p.id, getClk());
    }
}

void RR()
{
    // Initialize message queue
    printf("RR: Starting Algorthim...\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");
    sch_child_msgq_id = prepareMessageQueue("keys/sch_child_msgq_key");

    struct msgbuff message;

    // Initialize array
    Array *processes = (Array *)malloc(sizeof(Array));
    initArray(processes, 0);

    bool allProcessesSentFlag = false;
    bool processRunningFlag = false;
    int processToRun = 0;
    int quantum = 2, quantumCounter = 0;

    int status;

    while (!isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag)
    {
        // Receive a process from the message queue
        // printf("time before reading from message queue: %d\n", getClk());
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            // printf("Downed gen_sch_sem_id\n");
            addProcessesToArray(msgq_id, &message, processes, &allProcessesSentFlag);
        }

        printf("RR: Running..\n");
        // printf("time after reading from message queue: %d\n", getClk());

        // Check if the CPU is idle and there is a process to run
        // printf("RR: Entering Running Zone\n");
        if (!isArrEmpty(processes) && !processRunningFlag)
        {
            // printf("RR: Trying to run process %d\n", processToRun);
            struct process *nextProcess = processes->data[processToRun];
            // printf("process pid before fork: %d\n", nextProcess->pid);
            if (nextProcess->pid == -1)
                nextProcess->pid = fork();

            // printf("process pid after fork: %d\n", nextProcess->pid);

            if (nextProcess->pid == 0)
            {
                // printf("RR: Running process %d\n", nextProcess->id);
                while (nextProcess->remainingTime != 0)
                {
                    // if message received from scheduler continue executing
                    printf("Process %d running at time %d\n", nextProcess->id, getClk());
                    nextProcess->remainingTime--;

                    int currTime = getClk();
                    while (getClk() == currTime)
                    {
                    }
                }
                free(nextProcess);
                exit(0);
            }
            else
            {
                processRunningFlag = true;
                printf("Process %d continued at time %d\n", nextProcess->id, getClk());
                kill(nextProcess->pid, SIGCONT);
                quantumCounter = quantum;
            }
        }

        // printf("RR: Entering Waiting Zone\n");

        if (processRunningFlag)
        {
            quantumCounter--;
            // printf("quantumCounter: %d\n", quantumCounter);
            // printf("processRunningFlag: %d\n", processRunningFlag);
            if (quantumCounter == 0)
            {
                // printf("Entering quantum zone\n");
                // Check for process completion
                int child_pid = processes->data[processToRun]->pid;
                checkForProcessCompletion(&processRunningFlag, &child_pid);

                // TODO: Remember to not stop if process->size == 1
                if (processRunningFlag)
                {
                    printf("Process %d stopped at time %d\n", processes->data[processToRun]->id, getClk());
                    kill(processes->data[processToRun]->pid, SIGTSTP);
                    processToRun = (processToRun + 1) % processes->size;
                }
                else
                {
                    removeElement(processes, processToRun);
                    processToRun = processToRun % processes->size;
                    // checkForProcessCompletion -> changes processRunningFlag
                }
            }
        }
        // printf("RR: %d\n",  !allProcessesSentFlag );
        if (allProcessesSentFlag)
        {
            int currTime = getClk();
            while (currTime == getClk())
            {
            }
        }
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);

    // Free priority queue since it was dynamically allocated
    freeArray(processes);
}
