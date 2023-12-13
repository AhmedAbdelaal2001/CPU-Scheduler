#include "headers.h"
#include "priority_queue.h"
#include "Array.h"
#include "highest_priority_first.h"
#include "shortest_remaining_time_next.h"
#include "RR.h"

int main(int argc, char *argv[])
{
    initClk();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    int schedulingAlgorithm = atoi(argv[1]);
    int quantum = atoi(argv[2]);

    if (schedulingAlgorithm == 1)
        HPF();
    else if (schedulingAlgorithm == 2)
        SRTN();
    else if (schedulingAlgorithm == 3)
        RR(quantum);
    else
        printf("Invalid Input\n");

    destroyClk(true);
    exit(0);

    return 0;
}
