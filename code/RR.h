void RR_checkForProcessCompletion(Array *processes, struct process **runningProcess, int *processToRun, int *quantumRemainingTime, int quantum)
{
    int status;
    // Check for process completion
    if (*runningProcess)
    {
        if (waitpid((*runningProcess)->pid, &status, WNOHANG) > 0)
        {
            // Child process finished
            *runningProcess = NULL;
            removeElement(processes, *processToRun);
            *processToRun = (*processToRun) % processes->size;
            *quantumRemainingTime = quantum;
        }
    }
}

void RR_DetectAndHandlePreemption(Array *processes, struct process **runningProcess, int *processToRun, int *quantumRemainingTime, int quantum)
{
    if (*runningProcess && !isArrEmpty(processes))
    {
        if (*quantumRemainingTime == 0)
        {
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
        setProcessInformation(newProcess, message.p.id, message.p.arrival, message.p.runtime, message.p.priority);

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

void RR(int quantum)
{
    // Initialize message queue
    printf("RR: Starting Algorthim...\n\n");
    int msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    int sch_child_msgq_id = prepareMessageQueue("keys/sch_child_msgq_key");
    int gen_sch_sem_id = getSemaphore("keys/gen_sch_sem_key");

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
        RR_checkForProcessCompletion(processes, &runningProcess, &processToRun, &quantumRemainingTime, quantum);

        // Check for any preemptions
        RR_DetectAndHandlePreemption(processes, &runningProcess, &processToRun, &quantumRemainingTime, quantum);

        // Check if the CPU is idle and there is a process to run
        if ((!runningProcess && !isArrEmpty(processes)))
        {
            runningProcess = processes->data[processToRun];
            if (runningProcess->pid == -1)
            {
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
            quantumRemainingTime--;
        }
    }

    // Clean up message queue
    msgctl(msgq_id, IPC_RMID, NULL);
    msgctl(sch_child_msgq_id, IPC_RMID, NULL);
    // Free priority queue since it was dynamically allocated
    freeArray(processes);
}
