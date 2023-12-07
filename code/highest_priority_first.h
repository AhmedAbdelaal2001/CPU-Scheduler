#include "priority_queue.h"

int prepareMessageQueue() {
    
    key_t key_id;
    int msgq_id;

    key_id = ftok("keys/gen_sch_msg_key", 'M');   // use unique key
    msgq_id = msgget(key_id, 0666 | IPC_CREAT); // create message queue and return id

    if (msgq_id == -1)
    {
        perror("Error in create");
        exit(-1);
    }

    return msgq_id;

}

void checkForProcessCompletion(bool* processRunningFlag, pid_t* child_pid) {
    int status;
    // Check for process completion
    if (*processRunningFlag && *child_pid > 0)
    {
        if (waitpid(*child_pid, &status, WNOHANG) > 0)
        {
            // Child process finished
            *processRunningFlag = false;
            *child_pid = -1;
        }
    }
}

void receiveProcess(struct msgbuff message, PriorityQueue* priorityQueue, bool* allProcessesSentFlag) {
    if (message.mtype == TERMINATION_MSG_TYPE)
        *allProcessesSentFlag = true; //All processes have been received
    else{
        // Create a process pointer
        struct process* newProcess = (struct process*)malloc(sizeof(struct process));
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority);
        // Insert the process into the priority queue
        minHeapInsert(priorityQueue, newProcess);
    }
}

void HPF() {
    // Initialize message queue
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

        // Check for process completion
        checkForProcessCompletion(&processRunningFlag, &child_pid);

        // Receive a process from the message queue
        if (!allProcessesSentFlag)
            if (msgrcv(msgq_id, &message, sizeof(message.p), 0, IPC_NOWAIT) != -1)
                // Message received successfully
                receiveProcess(message, priorityQueue, &allProcessesSentFlag);

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
