#include "headers.h"

Block* memory = NULL; // The head of the Linked List

// Function to initialize memory
void initializeMemory(int totalSize) {
    memory = (Block*)malloc(sizeof(Block));
    
    memory->size = totalSize; // At the beginning, there is only one block, and its size equals the total available memory space
    memory->index = 1;  // The first node is the root of the tree. Hence, it has the index 1, just like the root of a binary heap.
    memory->isFree = 1; // No processes have been allocated yet.
    memory->next = NULL;
    memory->prev = NULL;
}

bool isBetterFit(Block* current, Block* bestFit) {
    return !bestFit || current->size < bestFit->size;
}

// Function to find the best fit block for the requested size
Block* findBestFit(int size) {
    Block* bestFit = NULL;
    Block* current;
    
    // Loop over all blocks
    // If a free block is found with size bigger than the process we are allocating that is a better/tighter fit than the current bestFit, make that block the new bestFit.
    for (current = memory; current != NULL; current = current->next)
        if (current->isFree && current->size >= size && isBetterFit(current, bestFit)) bestFit = current;
    
    return bestFit;
}

// Function to split the block
void splitBlock(Block* block, int size) {
    int halfSize = block->size >> 1;
    while (halfSize >= size) {
        // If the block can be split, create a new block such that if the old configuration is:
        // prev --> block --> next
        // Then the new configuration will be:
        // prev --> block --> newBlock --> next
        // Where block and new block have half the original size.

        // Note: we can treat the block and newBlock as the left and write children of the original block. We will only store the leaves instead of storing
        // the entire tree, but we can easily retrieve the parent by using the index data member, just like the way it is used in binary heaps.
        Block* newBlock = (Block*)malloc(sizeof(Block));
        newBlock->size = halfSize;
        newBlock->index = (block->index << 1) + 1;  // Right child
        newBlock->isFree = 1;
        newBlock->next = block->next;
        newBlock->prev = block;
        if (block->next) block->next->prev = newBlock;

        block->size = halfSize;
        block->index <<= 1;  // Left child
        block->next = newBlock;

        halfSize >>= 1;
    }
}

// Function to allocate memory
Block* allocate(int size) {

    Block* bestFit = findBestFit(size);

    if (bestFit == NULL) return NULL;  // No suitable block found

    // If the best fit was found, split it as much as possible, and make its "isFree" flag false.
    splitBlock(bestFit, size);
    bestFit->isFree = 0;
    return bestFit;
}

// Checks if two blocks/nodes are buddies or not
bool areBuddies(Block* block, Block* current) {
    // Two blocks are buddies iff they both have the same parent
    return (current->index >> 1) == (block->index >> 1);
}

// Function to find buddy
Block* findBuddy(Block* block) {
    // If a buddy exists, then it must be one of the neighbouring blocks
    Block* prev = block->prev;
    Block* next = block->next;
    
    // If any one of the neighbours shares the same parent with the block, then it is the buddy we are looking for
    if (prev && prev->isFree && areBuddies(block, prev)) return prev;
    if (next && next->isFree && areBuddies(block, next)) return next;
    return NULL;
}

// Function to merge buddy blocks
void mergeBuddies(Block* block) {
    while (block->size != MAX_MEM_SIZE) {
        Block* buddy = findBuddy(block);

        if (buddy == NULL) break; // No buddy found or buddy is not free

        Block* firstBlock;
        Block* secondBlock;

        // Order the block and its buddy
        if (buddy->index < block->index) {
            firstBlock = buddy;
            secondBlock = block;
        }
        else {
            firstBlock = block;
            secondBlock = buddy;
        }

        // Merge the two blocks
        firstBlock->size <<= 1;
        firstBlock->index >>= 1;
        firstBlock->next = secondBlock->next;
        free(secondBlock);

        // Handle neighbour
        if (firstBlock->next) firstBlock->next->prev = firstBlock;
        
        // Prepare for the next iteration
        block = firstBlock;
    }
}

// Function to deallocate memory
void deallocate(Block* block) {
    
    if (!block) return;

    block->isFree = 1; // Make the block free
    mergeBuddies(block); // Try merging with buddy
}

// Recrusively deletes the contents of the linked list
void freeLinkedList(Block* currBlock) {
    if (currBlock->next) freeLinkedList(currBlock->next);
    free(currBlock);
}

// Function to print the memory contents
void printMemoryContents() {
    Block* current;
    printf("Memory Contents:\n");
    printf("Size\tStatus\n");
    for (current = memory; current != NULL; current = current->next) {
        printf("%d\t%s\n", current->size, current->isFree ? "Free" : "Allocated");
    }
    printf("\n");
}


