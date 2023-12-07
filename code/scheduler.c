#include "highest_priority_first.h"

int main(int argc, char *argv[])
{
    initClk();
    // TODO implement the scheduler :)
    // upon termination release the clock resources.

    int schedulingAlgorithm = atoi(argv[1]);
    if (schedulingAlgorithm == 1)
        HPF();
    else
        printf("Not Implemented Yet :(\n");

    // initialise message queue if it was not initialised before
    if (atoi(argv[1]) == 2)
    {
        // selected algorithm is SRTN
    }

    destroyClk(true);
    exit(0);

    return 0;
}
