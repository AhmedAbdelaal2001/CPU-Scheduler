#include "headers.h"
#include "priority_queue.h"
#include "Array.h"
#include "highest_priority_first.h"
#include "shortest_remaining_time_next.h"
#include "RR.h"

int sch_child_msgq_id;

void clearResources(int signum);

int main(int argc, char *argv[])
{

    signal(SIGINT, clearResources);
    initClk();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    int schedulingAlgorithm = atoi(argv[1]);
    int quantum = atoi(argv[2]);
    sch_child_msgq_id = prepareMessageQueue("keys/sch_child_msgq_key");

    if (schedulingAlgorithm == 1)
        HPF(sch_child_msgq_id);
    else if (schedulingAlgorithm == 2)
        SRTN(sch_child_msgq_id);
    else if (schedulingAlgorithm == 3)
        RR(quantum, sch_child_msgq_id);
    else
        printf("Invalid Input\n");

    destroyClk(true);
    exit(0);

    return 0;
}

void clearResources(int signum)
{
    // Clear IPC resources
    // Terminate child processes
    // Handle other cleanup tasks
    // remove the message queue
    if (msgctl(sch_child_msgq_id, IPC_RMID, NULL) == -1)
    {
        perror("Error removing Scheduler-Child Message Queue");
    }

    exit(signum);
}
