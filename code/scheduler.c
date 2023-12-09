// #include "highest_priority_first.h"
#include "shortest_remaining_time_next.h"

int main(int argc, char *argv[])
{
    printf("SCHEDULER1: started\n");
    initClk();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.
    printf("SCHEDULER2: started\n");
    int schedulingAlgorithm = atoi(argv[1]);

    if (schedulingAlgorithm == 1)
        // HPF();
        printf("Not Implemented Yet :(\n");
    else if (schedulingAlgorithm == 2)
        SRTN();
    // else if (schedulingAlgorithm == 3)
    // RR();fsfhfkjhfouefoed
    else
        printf("Not Implemented Yet :(\n");

    destroyClk(true);
    exit(0);

    return 0;
}
