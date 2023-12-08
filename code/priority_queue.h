#include "headers.h"

void printHeapContents(PriorityQueue *pq)
{
    for (int i = 0; i < pq->size; i++)
        printf("Process %d has priority %d\n", pq->processArray[i]->id, pq->processArray[i]->priority);
}
void swap(struct process **processArray, int i, int j)
{
    struct process *tempProcess = processArray[i];
    processArray[i] = processArray[j];
    processArray[j] = tempProcess;
}

void initializePriorityQueue(PriorityQueue *pq) { pq->size = 0; }
struct process *heapMinimum(PriorityQueue *pq) { return pq->processArray[0]; }
bool isEmpty(PriorityQueue *pq) { return pq->size == 0; }

int parent(int i) { return (i - 1) / 2; }
int left(int i) { return 2 * i + 1; }
int right(int i) { return 2 * i + 2; }

void minHeapify(PriorityQueue *pq, int i)
{
    int l = left(i);
    int r = right(i);
    int smallest = i;

    if (l < pq->size && pq->processArray[l]->priority < pq->processArray[smallest]->priority)
        smallest = l;
    if (r < pq->size && pq->processArray[r]->priority < pq->processArray[smallest]->priority)
        smallest = r;

    if (smallest != i)
    {
        swap(pq->processArray, i, smallest);
        minHeapify(pq, smallest);
    }
}

struct process *heapExtractMin(PriorityQueue *pq)
{
    if (pq->size == 0)
        return NULL;
    struct process *min = pq->processArray[0];
    pq->processArray[0] = pq->processArray[pq->size - 1];
    pq->size--;

    minHeapify(pq, 0);
    return min;
}

void heapDecreaseKey(PriorityQueue *pq, int i, int key)
{
    if (key > pq->processArray[i]->priority)
        return;
    pq->processArray[i]->priority = key;

    while (i > 0 && pq->processArray[parent(i)]->priority > pq->processArray[i]->priority)
    {
        swap(pq->processArray, i, parent(i));
        i = parent(i);
    }
}

void minHeapInsert(PriorityQueue *pq, struct process *newProcess)
{
    if (pq->size == MAX_PROCESSES)
        return;
    pq->size++;
    pq->processArray[pq->size - 1] = newProcess;
    heapDecreaseKey(pq, pq->size - 1, newProcess->priority);
}