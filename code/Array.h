typedef struct
{
    struct process **data; // Pointer to dynamically allocated array
    int size;              // Current size of the array
    int capacity;          // Maximum capacity of the array
} Array;

// Function to initialize the array
void initArray(Array *arr, int initialCapacity)
{
    arr->data = (struct process **)malloc(initialCapacity * sizeof(struct process *));
    arr->size = 0;
    arr->capacity = initialCapacity;
}

// Function to resize the array
void resizeArray(Array *arr, int newCapacity)
{
    struct process **newData = (struct process **)malloc(newCapacity * sizeof(struct process *));
    for (int i = 0; i < arr->size; i++)
    {
        newData[i] = arr->data[i];
    }
    free(arr->data);
    arr->data = newData;
    arr->capacity = newCapacity;
}

// Function to check if the array is empty
int isArrEmpty(Array *arr)
{
    return arr->size == 0;
}

// Function to add an element to the array
void addElement(Array *arr, struct process *element)
{
    if (arr->size == arr->capacity)
    {
        resizeArray(arr, arr->capacity * 2); // Double the capacity if the array is full
    }
    arr->data[arr->size] = element;
    arr->size++;
}

// Function to update an element in the array
void updateElement(Array *arr, int index, struct process *element)
{
    if (index < 0 || index >= arr->size)
        return;

    arr->data[index] = element;
}

// Function to remove an element from the array
void removeElement(Array *arr, int index)
{
    if (index < 0 || index >= arr->size)
        return;

    for (int i = index; i < arr->size - 1; i++)
    {
        arr->data[i] = arr->data[i + 1];
    }
    arr->size--;
}

// Function to free the memory allocated for the array
void freeArray(Array *arr)
{
    free(arr->data);
    arr->size = 0;
    arr->capacity = 0;
}
