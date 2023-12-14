#include "headers.h"

int msgq_id;
int gen_sch_sem_id;

void clearResources(int);

int readInputFile(char *filename, struct process *processes)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int num_processes = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '#')
            continue; // Skip comment lines

        // Read process parameters
        int id, arrival, runtime, priority;
        if (sscanf(line, "%d\t%d\t%d\t%d", &id, &arrival, &runtime, &priority) == 4)
        {
            struct process p = create_process(id, arrival, runtime, priority);
            processes[num_processes++] = p;
        }
    }
    fclose(file);
    return num_processes;
}

void getSchedulingParameters(int *scheduling_algo, int *quantum)
{

    printf("Choose scheduling algorithm: \n");
    printf("1. Non-preemptive Highest Priority First (HPF)\n");
    printf("2. Shortest Remaining time Next (SRTN)\n");
    printf("3. Round Robin (RR)\n");
    printf("Enter your choice: ");
    scanf("%d", scheduling_algo);

    // Additional parameters for certain algorithms, like time quantum for RR
    if (*scheduling_algo == 3)
    { // Assuming 3 corresponds to RR
        printf("Enter time quantum: ");
        scanf("%d", quantum);
    }
}

pid_t createSchedulerProcess(int scheduling_algo, int quantum)
{
    // Parent process: continue creating the scheduler process
    pid_t scheduler_pid = fork();
    if (scheduler_pid == -1)
    {
        // Fork failed
        perror("Error forking scheduler process");
        exit(EXIT_FAILURE);
    }
    else if (scheduler_pid == 0)
    {
        // Child process: execute the scheduler program
        // Pass the chosen scheduling algorithm and quantum (if necessary) as arguments
        char scheduling_algo_str[2];
        char quantum_str[10];
        sprintf(scheduling_algo_str, "%d", scheduling_algo);
        sprintf(quantum_str, "%d", quantum);
        execl("./scheduler.out", "scheduler.out", scheduling_algo_str, quantum_str, (char *)NULL);
        // If execl returns, it has failed
        perror("Error executing scheduler program");
        exit(EXIT_FAILURE);
    }

    return scheduler_pid;
}

void createClockProcess()
{
    pid_t clk_pid = fork();
    if (clk_pid == -1)
    {
        // Fork failed
        perror("Error forking clock process");
        exit(EXIT_FAILURE);
    }
    else if (clk_pid == 0)
    {
        // Child process: execute the clock program
        execl("./clk.out", "clock.out", (char *)NULL);
        // If execl returns, it has failed
        perror("Error executing clock program");
        exit(EXIT_FAILURE);
    }
}

void sendProcessesToScheduler(int num_processes, struct process *processes, int scheduler_pid)
{
    struct msgbuff message;
    for (int i = 0; i < num_processes; i++)
    {
        int currTime = getClk();
        int nextArrivalTime = processes[i].arrival;

        while (currTime < nextArrivalTime)
        {
            up(gen_sch_sem_id);

            currTime = getClk();
            while (currTime == getClk())
            {
            }
            currTime++;
        }

        while (currTime == processes[i].arrival)
        {
            // send the process to the scheduler
            message.mtype = 1;
            message.p = processes[i];
            if (msgsnd(msgq_id, &message, sizeof(message.p), !IPC_NOWAIT) == -1)
            {
                perror("Error in sending message");
                exit(-1);
            }
            i++;

            if (i == num_processes)
            {
                // Send the termination message
                message.mtype = TERMINATION_MSG_TYPE; // Assuming TERMINATION_MSG_TYPE is defined
                msgsnd(msgq_id, &message, sizeof(message.p), !IPC_NOWAIT);
                break;
            }
        }
        up(gen_sch_sem_id);

        currTime = getClk();
        while (currTime == getClk())
        {
        }
        i--;
    }
}

int main(int argc, char *argv[])
{

    signal(SIGINT, clearResources);
    gen_sch_sem_id = prepareSemaphore("keys/gen_sch_sem_key", 0);

    // 1. Read the Input data from the file.
    struct process processes[MAX_PROCESSES];
    int num_processes = readInputFile("processes5.txt", processes);

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int scheduling_algo = 0;
    int quantum = 0;
    getSchedulingParameters(&scheduling_algo, &quantum);

    // 3. Initiate and create the scheduler and clock processes.
    pid_t scheduler_pid = createSchedulerProcess(scheduling_algo, quantum);
    createClockProcess();

    // 4. Use this function after creating the clock process to initialize clock
    initClk();

    // Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // (Assuming you have a message queue or other IPC mechanism set up for this)

    // create message queue between process_generator and scheduler
    msgq_id = prepareMessageQueue("keys/gen_sch_msg_key");
    sendProcessesToScheduler(num_processes, processes, scheduler_pid);

    // Wait for the scheduler process to finish
    int status;
    waitpid(scheduler_pid, &status, 0);

    // 7. Clear clock resources
    destroyClk(true);

    return 0;
}

void clearResources(int signum)
{
    // Clear IPC resources
    // Terminate child processes
    // Handle other cleanup tasks
    // remove the message queue
    if (msgctl(msgq_id, IPC_RMID, NULL) == -1)
    {
        perror("Error removing message queue");
    }

    if (semctl(gen_sch_sem_id, 0, IPC_RMID, NULL) == -1)
    {
        perror("Error removing message queue");
    }
    exit(signum);
}