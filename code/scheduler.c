#include "highest_priority_first.h"
#include "RR.h"


int main(int argc, char *argv[])
{
    initClk();

    // TODO implement the scheduler :)
    // upon termination release the clock resources.
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
