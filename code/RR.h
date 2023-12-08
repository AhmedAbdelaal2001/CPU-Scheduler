#include "Array.h"

void AddProcessToArray(struct msgbuff message, Array *arr, bool *allProcessesSentFlag)
{
    if (message.mtype == TERMINATION_MSG_TYPE)
        *allProcessesSentFlag = true; // All processes have been received
    else
    {
        // Create a process pointer
        struct process *newProcess = (struct process *)malloc(sizeof(struct process));
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority);
        // Insert the process into the array
        addElement(arr, newProcess);
    }
}

void RR()
{
    int msgq_id = prepareMessageQueue();

    struct msgbuff message;

    // Create array for processes
    Array *processes = (Array *)malloc(sizeof(Array));
    initArray(processes, 0);

    bool allProcessesSentFlag = false;
    bool processRunningFlag = false;

    int status, processToRun = 0, quantum = 1;

    while (!isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag)
    {
        printf("AHOO1: %d\n", !isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag);

        // Receive a process from the message queue
        if (!allProcessesSentFlag && doneSending)
            receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag);

        printf("AHOO2: %d\n", !isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag);

        // Get the next process to run
        struct process *process = processes->data[processToRun];
        if (process->pid == -1)
            process->pid = fork();
        printf("AHOO3: %d\n", !isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag);
        if (process->pid == 0)
        {
            // Child process: simulate process execution
            int prevTime = getClk();
            printf("Process %d running at time %d\n", process->id, prevTime);

            while (process->remainingTime != 0)
            {
                if (getClk() != prevTime)
                {
                    if (process->remainingTime != 1)
                        printf("Process %d running at time %d\n", process->id, getClk());
                    prevTime = getClk();
                    process->remainingTime--;
                }
            }
            exit(0);
        }
        else
        {
            // Update CPU state to idle
            kill(process->pid, SIGCONT);
            processRunningFlag = true;

            printf("Process %d resumed at time %d\n", process->id, getClk());
            int startTime = getClk();
            while (getClk() - startTime < quantum)
            {
            }
            printf("Deciding...\n");

            checkForProcessCompletion(&processRunningFlag, &process->pid);
            printf("processRunningFlag: %d\n", processRunningFlag);
            if (processRunningFlag)
            {
                printf("Process %d stopped at time %d\n", process->id, getClk());
                kill(process->pid, SIGSTOP);
                processToRun = (processToRun + 1) % processes->size;
                printf("Next process to run: %d\n", processToRun + 1);
            }
            else
            {
                printf("Process %d finished at time %d\n", process->id, getClk());
                removeElement(processes, processToRun);
                processRunningFlag = processes->size != 0;
                processToRun = processToRun % processes->size;
            }
        }

        printf("%d\n", !isArrEmpty(processes) || !allProcessesSentFlag || processRunningFlag);
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);

    // Free priority queue since it was dynamically allocated
    freeArray(processes);
}