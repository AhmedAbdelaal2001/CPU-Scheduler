#include "priority_queue.h"

void main() {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    initializePriorityQueue(pq);

    struct process* newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 1, 1, 6, 5);
    minHeapInsert(pq, newProcess);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 2, 3, 3, 3);
    minHeapInsert(pq, newProcess);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 3, 3, 3, 4);
    minHeapInsert(pq, newProcess);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 4, 3, 3, 1);
    minHeapInsert(pq, newProcess);

    heapExtractMin(pq);
    heapExtractMin(pq);
    heapExtractMin(pq);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 5, 3, 3, 10);
    minHeapInsert(pq, newProcess);

    heapDecreaseKey(pq, 1, 3);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 6, 3, 3, 6);
    minHeapInsert(pq, newProcess);

    newProcess = (struct process*)malloc(sizeof(struct process));
    setProcessInformation(newProcess, 7, 3, 3, 2);
    minHeapInsert(pq, newProcess);
    
    printHeapContents(pq);

}