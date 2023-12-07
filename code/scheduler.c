#include "highest_priority_first.h"

int main(int argc, char *argv[])
{
    initClk();
    // TODO implement the scheduler :)
    // initialise message queue if it was not initialised before
    key_t msgq_key = ftok("keys/gen_sch_msg_key", 'M');
    int msgq_id = msgget(msgq_key, IPC_CREAT | 0644);
    if (*argv[1] == 2)
    {
        // selected algorithm is SRTN
        while (1)
        {
            // check on message queue for new processes
        }
    }
    // upon termination release the clock resources.

    int schedulingAlgorithm = atoi(argv[1]);
    if (schedulingAlgorithm == 1)    
        HPF();
    else
        printf("Not Implemented Yet :(\n");
    
    destroyClk(true);
    exit(0);

    return 0;

}
