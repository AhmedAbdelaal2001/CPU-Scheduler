#include "buddyMemoryAllocation.h"

int main() {
    initializeMemory(MAX_MEM_SIZE);
    printMemoryContents();


    //// Example usage 1
    //Block* ptrA = allocate(70);
    //printMemoryContents();
    //Block* ptrB = allocate(35);
    //printMemoryContents();
    //Block* ptrC = allocate(80);
    //printMemoryContents();
    //deallocate(ptrA);
    //printMemoryContents();
    //Block* ptrD = allocate(60);
    //printMemoryContents();
    //deallocate(ptrB);
    //printMemoryContents();
    //deallocate(ptrD);
    //printMemoryContents();
    //deallocate(ptrC);
    //printMemoryContents();

    // Example usage 2
    Block* ptrA = allocate(5);
    printMemoryContents();
    Block* ptrB = allocate(25);
    printMemoryContents();
    Block* ptrC = allocate(20);
    printMemoryContents();
    Block* ptrD = allocate(23);
    printMemoryContents();
    deallocate(ptrB);
    printMemoryContents();
    deallocate(ptrC);
    printMemoryContents();
    deallocate(ptrD);
    printMemoryContents();
    deallocate(ptrA);
    printMemoryContents();

    freeLinkedList(memory);
    return 0;
}