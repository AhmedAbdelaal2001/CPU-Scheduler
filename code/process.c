#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char *argv[])
{
    initClk();

    // receive id and remaining time from scheduler
    int id = atoi(argv[1]);
    int remainingTime = atoi(argv[2]);

    // TODO it needs to get the remaining time from somewhere
    int sch_child_msgq_id = prepareMessageQueue("keys/sch_child_msgq_key");
    struct msgbuff sch_child_message;
    while (remainingTime != 0)
    {
        msgrcv(sch_child_msgq_id, &sch_child_message, sizeof(sch_child_message.p), getpid(), !IPC_NOWAIT);
        printf("Process %d running at time %d\n", id, getClk());
        remainingTime--;
    }

    destroyClk(false);
    exit(0);
}
