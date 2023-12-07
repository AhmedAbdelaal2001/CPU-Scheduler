// SRTN Implementaion
#include "highest_priority_first.h"

// called after insertion of process
void insertAndSortProcess(struct process *SRTN, int size, struct process newProcess, int msgq_id)
{
    SRTN = (struct process *)realloc(SRTN, sizeof(struct process) * (size + 1));
    // insert process in the correct position in the array
    int i;
    for (i = 0; i < (size - 1); i++)
    {
        if (SRTN[i].runtime > newProcess.runtime)
            break;
    }
    // shift all processes after the new process to the right
    for (int j = size; j > i; j--)
        SRTN[j] = SRTN[j - 1];
    // insert the new process
    SRTN[i] = newProcess;
}

void deleteProcess(struct process *SRTN, int size, int index)
{
    // shift all processes after the deleted process to the left
    for (int i = index; i < size - 1; i++)
        SRTN[i] = SRTN[i + 1];
    // reallocate memory
    SRTN = (struct process *)realloc(SRTN, sizeof(struct process) * (size - 1));
}

void SRTN()
{
    // initialise message queue if not initialised
    int msgq_id = prepareMessageQueue();
    int processToRun;

    // initialise SRTN array dynamically
    struct process *SRTN = (struct process *)malloc(sizeof(struct process));
    int SRTN_SIZE = 0;

    while (1)
    {
        // receive process from queue if arrived
        struct msgbuff message;
        if (msgrcv(msgq_id, &message, sizeof(message.p), 0, IPC_NOWAIT) != -1)
        {
            // insert process in the correct position in the array
            insertAndSortProcess(SRTN, SRTN_SIZE, message.p, msgq_id);
            SRTN_SIZE++;

            // to initialise the processToRun variable
            if (SRTN_SIZE == 1)
            {
                processToRun = SRTN[0].id;
            }
        }

        // check if there are any processes
        if (SRTN_SIZE == 0)
        {
            continue;
        }

        else
        {
            // check if the process is finished
            if (SRTN[0].runtime == 0)
            {
                // delete the process from the array
                deleteProcess(SRTN, SRTN_SIZE, 0);
                SRTN_SIZE--;
            }

            // check if there is a process running
            if (processToRun == SRTN[0].id)
            {
                // run the process
                processToRun = SRTN[0].id;
                // SRTN[0].runtime--;
            }

            else
            {
                // check if the process is the same as the running process
                if (processToRun == SRTN[0].id)
                {
                    // continue running the process
                    // SRTN[0].runtime--;
                }

                else
                {
                    // stop the running process
                    kill(processToRun, SIGSTOP);
                    // run the new process
                    processToRun = SRTN[0].id;
                    kill(processToRun, SIGCONT);
                    // SRTN[0].runtime--;
                }
            }
        }
    }
}