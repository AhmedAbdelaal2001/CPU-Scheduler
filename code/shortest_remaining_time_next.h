#include "priority_queue.h"

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

void storeLPerfAndLogFiles(struct log *logArray, int logArraySize)
{
    // Initialize average waiting time and average weighted turnaround time
    float avgWaitingTime = 0;
    float avgWeightedTurnaroundTime = 0;
    // Initialize array of weighted turnaround times
    float *weightedTurnAroundTimes = (float *)malloc(logArraySize * sizeof(float));

    int countTurnAround = 0;
    int countWaiting = 0;
    // Print the log array
    // Initialize the log file
    FILE *logFile = fopen("scheduler.log", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    for (int i = 0; i < logArraySize; i++)
    {
        // state = 0 for started, 1 for stopped, 2 for resumed, 3 for finished
        if (logArray[i].state == 3) // process is finished. Need Turnaround time and weighted turnaround time
        {
            logArray[i].currTime--;
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
            // if a process resumes
            if (logArray[i].state == 2)
            {
                // Add the waiting time to the average
                countWaiting++;
                avgWaitingTime += logArray[i].waitTime;
            }
            fprintf(logFile, "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
                    logArray[i].currTime, logArray[i].id, logArray[i].state == 0 ? "started" : logArray[i].state == 1 ? "stopped"
                                                                                           : logArray[i].state == 2   ? "resumed"
                                                                                                                      : "finished",
                    logArray[i].arrivalTime, logArray[i].runTime, logArray[i].remainingTime, logArray[i].waitTime);
        }
    }
    // Calculate the average waiting time and average weighted turnaround time
    fclose(logFile);
    avgWaitingTime /= countWaiting; // needs revising
    avgWeightedTurnaroundTime /= countTurnAround;

    // Compute the standard deviation
    float standardDeviation = 0;
    for (int i = 0; i < countTurnAround; i++)
    {
        standardDeviation += pow(weightedTurnAroundTimes[i] - avgWeightedTurnaroundTime, 2);
    }

    // Store the performance metrics
    FILE *performanceFile = fopen("scheduler.perf", "w");
    // Add CPU utilization to the file
    // fprintf(performanceFile, "CPU utilization = %.2f%%\n", (float)(logArray[logArraySize - 1].currTime - logArray[0].currTime) / logArray[logArraySize - 1].currTime * 100);
    // Add average waiting time and average weighted turnaround time to the file
    fprintf(performanceFile, "Average Waiting Time = %.2f\n", avgWaitingTime);
    fprintf(performanceFile, "Average Weighted Turnaround Time = %.2f\n", avgWeightedTurnaroundTime);
    fprintf(performanceFile, "Standard Deviation = %.2f\n", sqrt(standardDeviation / countTurnAround));
    fclose(performanceFile);
}

void SRTN_checkForProcessCompletion(struct process **runningProcess, struct log **logArray, int *logArraySize)
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
            struct log Log = createLog((*runningProcess)->id, getClk(), 3, (*runningProcess)->arrival, (*runningProcess)->runtime, 0, 0);
            addLog(logArray, logArraySize, Log);
            *runningProcess = NULL;
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
            struct log Log = createLog((*runningProcess)->id, getClk(), 1, (*runningProcess)->arrival, (*runningProcess)->runtime, (*runningProcess)->remainingTime, 0);
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

void SRTN_receiveProcess(struct msgbuff message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
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
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.runtime);

        // Insert the process into the priority queue. Since the priority of the process is set to its runtime,
        // the priority queue will automatically handle the ordering of the values with respect to the running time.
        minHeapInsert(priorityQueue, newProcess);

        // printf("Received Message %d at time %d\n", message.p.id, getClk());
    }
}

void SRTN_receiveProcesses(int msgq_id, struct msgbuff *message, PriorityQueue *priorityQueue, bool *allProcessesSentFlag)
{
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        SRTN_receiveProcess(*message, priorityQueue, allProcessesSentFlag);
    }
}

void SRTN()
{
    // Initialize message queue
    printf("SRTN: Starting Algorthim...\n\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int sch_child_msgq_id = prepareMessageQueue("keys/sch_child_msgq_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");

    // Initialise log array
    int logArraySize = 0;
    struct log *logArray = createLogArray(logArraySize);

    struct msgbuff message;
    struct msgbuff sch_child_message;
    // Initialize priority queue
    PriorityQueue *priorityQueue = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    initializePriorityQueue(priorityQueue);

    bool allProcessesSentFlag = false;
    bool preemptionFlag = false;
    struct process *runningProcess = NULL;

    int status;
    pid_t child_pid = -1; // Track the PID of the running child process

    while (!isEmpty(priorityQueue) || !allProcessesSentFlag || runningProcess)
    {
        // Receive a process from the message queue
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            // printf("Down Completed\n");
            SRTN_receiveProcesses(msgq_id, &message, priorityQueue, &allProcessesSentFlag);
        }

        // Check for process completion
        SRTN_checkForProcessCompletion(&runningProcess, &logArray, &logArraySize);

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
                struct log Log = createLog(runningProcess->id, getClk(), 2, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->resumeTime - runningProcess->stopTime);
                addLog(&logArray, &logArraySize, Log);
            }
            if (runningProcess->pid == -1)
            {
                // Add the process to the log array
                struct log Log = createLog(runningProcess->id, getClk(), 0, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, 0);
                addLog(&logArray, &logArraySize, Log);

                // printf("Process %d will start running at time %d\n", runningProcess->id, getClk());
                runningProcess->pid = fork();
                if (runningProcess->pid == -1)
                    perror("Fork Falied");
                else if (runningProcess->pid == 0)
                    runProcess(runningProcess);
            }
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

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    msgctl(sch_child_msgq_id, IPC_RMID, NULL);
    // Free priority queue since it was dynamically allocated
    free(priorityQueue);
    printf("SRTN: Finished Algorthim...\n");

    // Store the log file
    storeLPerfAndLogFiles(logArray, logArraySize);
}
