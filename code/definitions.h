// Define a constant for the maximum number of processes
#define MAX_PROCESSES 100

// A special message type to indicate that all processes have been sent
#define TERMINATION_MSG_TYPE 999 

// common structs
struct process
{
    int id;
    int arrival;
    int runtime;
    int priority;

    int remainingTime;
};

struct msgbuff
{
    long mtype;
    struct process p;
};

typedef struct {
    struct process* processArray[MAX_PROCESSES];
    int size;
} PriorityQueue;

// Process Constructor
struct process create_process(int id, int arrival, int runtime, int priority)
{
    struct process p;
    p.id = id;
    p.arrival = arrival;
    p.runtime = runtime;
    p.priority = priority;

    p.remainingTime = runtime;
    return p;
}

void setProcessInformation(struct process* newProcess, int id, int arrival, int runtime, int priority) {
    newProcess->id = id;
    newProcess->arrival = arrival;
    newProcess->runtime = runtime;
    newProcess->priority = priority;

    newProcess->remainingTime = runtime;
}