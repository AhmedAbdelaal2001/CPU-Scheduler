#include "headers.h"
#include "definitions.h"

void clearResources(int);

void down(int sem, int amount)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -amount;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    FILE *file = fopen("processes.txt", "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    struct process processes[MAX_PROCESSES];
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

    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int scheduling_algo;
    printf("Choose scheduling algorithm: \n");
    printf("1. Non-preemptive Highest Priority First (HPF)\n");
    printf("2. Shortest Remaining time Next (SRTN)\n");
    printf("3. Round Robin (RR)\n");
    printf("Enter your choice: ");
    scanf("%d", &scheduling_algo);

    // Additional parameters for certain algorithms, like time quantum for RR
    int quantum = 0;
    if (scheduling_algo == 3)
    { // Assuming 3 corresponds to RR
        printf("Enter time quantum: ");
        scanf("%d", &quantum);
    }

    // 3. Initiate and create the scheduler and clock processes.
    pid_t pid = fork();
    if (pid == -1)
    {
        // Fork failed
        perror("Error forking clock process");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Child process: execute the clock program
        execl("./clk.out", "clock.out", (char *)NULL);
        // If execl returns, it has failed
        perror("Error executing clock program");
        exit(EXIT_FAILURE);
    }

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

    // 4. Use this function after creating the clock process to initialize clock
    // sleep(3);
    initClk();

    // Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // (Assuming you have a message queue or other IPC mechanism set up for this)

    printf("Entering main loop\n");

    // get the semaphore
    key_t sem_key = ftok("keys/clk_gen_sem_key", 'S');
    int sem_id = semget(sem_key, 1, IPC_CREAT | 0644);

    // create message queue between process_generator and scheduler
    key_t msgq_key = ftok("keys/gen_sch_msg_key", 'M');
    int msgq_id = msgget(msgq_key, IPC_CREAT | 0644);
    struct msgbuff message;

    for (int i = 0; i < num_processes; i++)
    {
        // down the semaphore with the difference between the arrival time of the current process and the arrival time of the previous process
        // if (i > 0) down(sem_id, processes[i].arrival - processes[i - 1].arrival);
        // else down(sem_id, processes[i].arrival);
        // printf("Process %d will arrive in %d\n", processes[i].id, processes[i].arrival - getClk());
        down(sem_id, processes[i].arrival - getClk());

        // send the process to the scheduler
        message.mtype = 1;
        message.p = processes[i];
        if (msgsnd(msgq_id, &message, sizeof(message.p), !IPC_NOWAIT) == -1)
        {
            perror("Error in sending message");
            exit(-1);
        }
        //msgrcv(msgq_id, &message, sizeof(message.p), 0, IPC_NOWAIT); 
        // print the process info
        //printf("Process %d sent to scheduler at time %d\n", message.p.id, getClk());
    }

    // Send the termination message
    message.mtype = TERMINATION_MSG_TYPE; // Assuming TERMINATION_MSG_TYPE is defined
    msgsnd(msgq_id, &message, sizeof(message.p), !IPC_NOWAIT);
    

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
    exit(signum);
}