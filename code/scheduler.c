#include "headers.h"
#include "highest_priority_first.h"
#include "RR.h"

// signal handler for SIGUSR1 for handling the arrival of a new process
void handler(int signum)
{
    printf("SCHEDULER: received SIGUSR1\n");
    doneSending = true;
    printf("SCHEDULER: finished handling SIGUSR1\n");
}

int main(int argc, char *argv[])
{
    printf("SCHEDULER1: started\n");
    initClk();

    signal(SIGUSR1, handler);

    // TODO implement the scheduler :)
    // upon termination release the clock resources.
    printf("SCHEDULER2: started\n");
    int schedulingAlgorithm = atoi(argv[1]);

    if (schedulingAlgorithm == 1)
        HPF();
    // else if (schedulingAlgorithm == 2)
    //     SRTN();
    else if (schedulingAlgorithm == 3)
        RR();
    else
        printf("Not Implemented Yet :(\n");

    destroyClk(true);
    exit(0);

    return 0;
}
