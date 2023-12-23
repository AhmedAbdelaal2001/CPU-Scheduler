// Function to initialize the array
void initMemoryLogArray(memoryLogArray *arr, int initialCapacity)
{
    arr->data = (memoryLog **)malloc(initialCapacity * sizeof(memoryLog *));
    arr->size = 0;
    arr->capacity = initialCapacity;
}

// Function to resize the array
void resizeMemoryLogArray(memoryLogArray *arr, int newCapacity)
{
    memoryLog **newData = (memoryLog **)malloc(newCapacity * sizeof(memoryLog *));
    for (int i = 0; i < arr->size; i++)
    {
        newData[i] = arr->data[i];
    }
    free(arr->data);
    arr->data = newData;
    arr->capacity = newCapacity;
}

// Function to check if the array is empty
int isMemoryLogArrayEmpty(memoryLogArray *arr)
{
    return arr->size == 0;
}

// Function to add an element to the array
void addElement_memoryLogArray(memoryLogArray *arr, memoryLog *element)
{
    if (arr->size == arr->capacity)
    {
        resizeMemoryLogArray(arr, arr->capacity * 2); // Double the capacity if the array is full
    }
    arr->data[arr->size] = element;
    arr->size++;
}

// Function to update an element in the array
void updateElement_memoryLogArray(memoryLogArray *arr, int index, memoryLog *element)
{
    if (index < 0 || index >= arr->size)
        return;

    arr->data[index] = element;
}

// Function to remove an element from the array
void removeElement_memoryLogArray(memoryLogArray *arr, int index)
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
void freeMemoryLogArray(memoryLogArray *arr)
{
    free(arr->data);
    arr->size = 0;
    arr->capacity = 0;
}
