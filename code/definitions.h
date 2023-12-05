// Define a constant for the maximum number of processes
#define MAX_PROCESSES 100

// common structs
struct process
{
    int id;
    int arrival;
    int runtime;
    int priority;
};

struct msgbuff
{
    long mtype;
    struct process p;
};

// Process Constructor
struct process create_process(int id, int arrival, int runtime, int priority)
{
    struct process p;
    p.id = id;
    p.arrival = arrival;
    p.runtime = runtime;
    p.priority = priority;
    return p;
}