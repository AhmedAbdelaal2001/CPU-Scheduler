// function to create an array of log structs
struct log *RR_createLogArray(int size)
{
    struct log *logArray = (struct log *)malloc(size * sizeof(struct log));
    return logArray;
}

// function to create a log struct
struct log RR_createLog(int id, int currTime, int state, int arrivalTime, int runTime, int remainingTime, int waitingTime)
{
    int turnAroundTime = currTime - arrivalTime;
    // weighted turnaround to the nearest 2 decimal places
    float weightedTurnAroundTime = (float)turnAroundTime / runTime;
    struct log Log = {id, currTime, state, arrivalTime, runTime, remainingTime,
                      waitingTime, turnAroundTime, weightedTurnAroundTime};
    return Log;
}

// function to add a log to the log array
void RR_addLog(struct log **logArray, int *size, struct log Log)
{
    // reallocating the array to add the new log
    *logArray = (struct log *)realloc(*logArray, (*size + 1) * sizeof(struct log));
    (*logArray)[*size] = Log;
    (*size)++;
}

void RR_storePerfAndLogFiles(struct log *logArray, int logArraySize, int idleCounter)
{
    // Initialize average waiting time and average weighted turnaround time
    float avgWaitingTime = 0;
    float avgWeightedTurnaroundTime = 0;
    // Initialize array of weighted turnaround times
    float *weightedTurnAroundTimes = (float *)malloc(logArraySize * sizeof(float));

    int countTurnAround = 0;
    int countWaiting = 0;

    int totalTime = 0;
    // Print the log array
    // Initialize the log file
    FILE *logFile = fopen("scheduler.log", "w");
    fprintf(logFile, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
    for (int i = 0; i < logArraySize; i++)
    {
        // state = 0 for started, 1 for stopped, 2 for resumed, 3 for finished
        if (logArray[i].state == 3) // process is finished. Need Turnaround time and weighted turnaround time
        {
            // Calculate average waiting time
            countWaiting++;
            avgWaitingTime += logArray[i].waitTime;

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
            fprintf(logFile, "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n",
                    logArray[i].currTime, logArray[i].id, logArray[i].state == 0 ? "started" : logArray[i].state == 1 ? "stopped"
                                                                                           : logArray[i].state == 2   ? "resumed"
                                                                                                                      : "finished",
                    logArray[i].arrivalTime, logArray[i].runTime, logArray[i].remainingTime, logArray[i].waitTime);
        }

        // Getting the total time by the last finsihed process
        if (i == logArraySize - 1)
        {
            totalTime = logArray[i].currTime - 1;
        }
    }
    // Calculate the average waiting time and average weighted turnaround time
    fclose(logFile);
    avgWeightedTurnaroundTime /= countTurnAround;

    if (countWaiting == 0) // special case when no process waits
        avgWaitingTime = 0;
    else
        avgWaitingTime /= countWaiting;

    // Compute the standard deviation
    float standardDeviation = 0;
    for (int i = 0; i < countTurnAround; i++)
    {
        standardDeviation += pow(weightedTurnAroundTimes[i] - avgWeightedTurnaroundTime, 2);
    }

    // Store the performance metrics
    FILE *performanceFile = fopen("scheduler.perf", "w");
    // Add CPU utilization to the file
    fprintf(performanceFile, "CPU utilization = %.2f%%\n", (float)(totalTime - idleCounter) / totalTime * 100);
    // Add average waiting time and average weighted turnaround time to the file
    fprintf(performanceFile, "Average Waiting = %.2f\n", avgWaitingTime);
    fprintf(performanceFile, "Average WTA = %.2f\n", avgWeightedTurnaroundTime);
    fprintf(performanceFile, "Std WTA = %.2f\n", sqrt(standardDeviation / countTurnAround));
    fclose(performanceFile);
}

void RR_checkForProcessCompletion(Array *processes, struct process **runningProcess, int *processToRun, int *quantumRemainingTime, int quantum, struct log **logArray, int *logArraySize)
{
    int status;
    // Check for process completion
    if (*runningProcess)
    {
        if (waitpid((*runningProcess)->pid, &status, WNOHANG) > 0)
        {
            // Create a finished log for the finished process
            struct log Log = RR_createLog((*runningProcess)->id, getClk(), 3, (*runningProcess)->arrival, (*runningProcess)->runtime, 0, (*runningProcess)->waitTime);
            RR_addLog(logArray, logArraySize, Log);

            // Child process finished
            *runningProcess = NULL;
            removeElement(processes, *processToRun);
            if (!isArrEmpty(processes))
            {
                *processToRun = (*processToRun) % processes->size;
                *quantumRemainingTime = quantum;
            }

            // printf("Process finished\n");
        }
    }
}

void RR_DetectAndHandlePreemption(Array *processes, struct process **runningProcess, int *processToRun, int *quantumRemainingTime, int quantum, struct log **logArray, int *logArraySize)
{
    if (*runningProcess && !isArrEmpty(processes))
    {
        if (*quantumRemainingTime == 0)
        {
            // Create a preemption log for stopped process
            struct log Log = RR_createLog((*runningProcess)->id, getClk(), 1, (*runningProcess)->arrival, (*runningProcess)->runtime, (*runningProcess)->remainingTime, (*runningProcess)->waitTime);
            RR_addLog(logArray, logArraySize, Log);
            // store stop time
            (*runningProcess)->stopTime = getClk();

            *processToRun = (*processToRun + 1) % processes->size;
            *runningProcess = NULL;
            *quantumRemainingTime = quantum;
        }
    }
}

void RR_receiveProcess(struct msgbuff message, Array *processes, bool *allProcessesSentFlag)
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
        addElement(processes, newProcess);
    }
}

void RR_receiveProcesses(int msgq_id, struct msgbuff *message, Array *processes, bool *allProcessesSentFlag)
{
    while (msgrcv(msgq_id, message, sizeof(message->p), 0, IPC_NOWAIT) != -1)
    {
        RR_receiveProcess(*message, processes, allProcessesSentFlag);
    }
}

void RR(int quantum, int sch_child_msgq_id)
{
    // Initialize message queue
    printf("RR: Starting Algorthim...\n\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");

    // Initialise log array
    int logArraySize = 0;
    struct log *logArray = RR_createLogArray(logArraySize);

    // Initialize idle counter and total time
    int idleCounter = 0;

    struct msgbuff message;
    struct msgbuff sch_child_message;

    // Initialize array
    Array *processes = (Array *)malloc(sizeof(Array));
    initArray(processes, 1);

    bool allProcessesSentFlag = false;
    bool preemptionFlag = false;
    struct process *runningProcess = NULL;
    int processToRun = 0;
    int quantumRemainingTime = quantum;

    int status;
    pid_t child_pid = -1; // Track the PID of the running child process

    while (!isArrEmpty(processes) || !allProcessesSentFlag || runningProcess)
    {
        // Receive a process from the message queue
        if (!allProcessesSentFlag)
        {
            down(gen_sch_sem_id);
            RR_receiveProcesses(msgq_id, &message, processes, &allProcessesSentFlag);
        }

        // Check for process completion
        RR_checkForProcessCompletion(processes, &runningProcess, &processToRun, &quantumRemainingTime, quantum, &logArray, &logArraySize);

        // Check for any preemptions
        RR_DetectAndHandlePreemption(processes, &runningProcess, &processToRun, &quantumRemainingTime, quantum, &logArray, &logArraySize);

        // Check if the CPU is idle and there is a process to run
        if ((!runningProcess && !isArrEmpty(processes)))
        {
            runningProcess = processes->data[processToRun];

            // Resume time log check
            if (runningProcess->remainingTime < runningProcess->runtime)
            {
                // Store resume time
                // printf("Resuming process %d\n", runningProcess->id);
                runningProcess->resumeTime = getClk();
                runningProcess->waitTime += runningProcess->resumeTime - runningProcess->stopTime;
                struct log Log = RR_createLog(runningProcess->id, getClk(), 2, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->waitTime);
                RR_addLog(&logArray, &logArraySize, Log);
            }

            if (runningProcess->pid == -1)
            {
                runningProcess->pid = fork();
                runningProcess->waitTime += getClk() - runningProcess->arrival;
                struct log Log = RR_createLog(runningProcess->id, getClk(), 0, runningProcess->arrival, runningProcess->runtime, runningProcess->remainingTime, runningProcess->waitTime);
                // Add the process to the log array
                RR_addLog(&logArray, &logArraySize, Log);

                if (runningProcess->pid == -1)
                    perror("Fork Falied");
                else if (runningProcess->pid == 0)
                    runProcess(runningProcess);
            }
        }
        else if (!runningProcess && isArrEmpty(processes))
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
            quantumRemainingTime--;
        }
    }
    // printf("All processes have finished\n");
    //  idleCounter - 2 if you want to exclude last loop where it makes sure all procs ended
    idleCounter = idleCounter - 2;
    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    msgctl(sch_child_msgq_id, IPC_RMID, NULL);
    // Free priority queue since it was dynamically allocated
    freeArray(processes);
    RR_storePerfAndLogFiles(logArray, logArraySize, idleCounter);
}
