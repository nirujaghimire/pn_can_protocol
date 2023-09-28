//
// Created by niruja on 9/23/2023.
//

#include <stdio.h>
#include "static_heap.h"

/**
 * This initiates the static heap
 * @param heap          : Pointer to static heap
 * @param map           : Pointer to statically allocated map
 * @param mapSize       : Max number of allocation
 * @param memory        : Pointer to statically allocated memory
 * @param memorySize    : Max size of memory
 */
static void init(Heap *heap, HeapMemoryMap *map, int mapSize, HeapMemory *memory, int memorySize) {
    heap->map = map;
    heap->memory = memory;
    heap->mapSize = mapSize;
    heap->memorySize = memorySize;
    heap->map[0].start = 0;
    heap->map[0].size = memorySize;
    heap->map[0].isAllocated = 0;
    heap->mapCounter = 1;
}

/**
 * Allocates memory of required size
 * @param heap          : Pointer to static heap
 * @param requiredSize  : Required size in byte
 * @return              : pointer to allocated memory (OR)
 *                          NULL if memory is not available
 */
static void *allocate(Heap *heap, int requiredSize) {
    int index = -1;//Index for map pointing allocated memory
    for (int i = 0; i < heap->mapCounter; ++i) {
        if (heap->map[i].size < requiredSize || heap->map[i].isAllocated)
            continue;
        heap->map[i].isAllocated = 1;//allocate memory
        index = i;
        break;
    }

    if (index == -1)// Memory is not available
        return NULL;

    //There is no room for another map
    if (heap->mapCounter == heap->mapSize || index == (heap->mapSize - 1))
        return &(heap->memory[heap->map[index].start]);


    //Check next map if it exists or not
    if ((index + 1) < heap->mapCounter) {
        //Next map exist
        int remainingFreeSpaceInBetween = heap->map[index].size - requiredSize;

        if (remainingFreeSpaceInBetween == 0)
            return &(heap->memory[heap->map[index].start]);//no need to create map in between

        //map in between is to be created
        //Shift all next maps to further one step
        for (int i = (heap->mapCounter - 1); i > (index - 1); --i)
            heap->map[i] = heap->map[i - 1];

        //Create in between map
        heap->map[index + 1].start = heap->map[index].start + requiredSize;
        heap->map[index + 1].size = remainingFreeSpaceInBetween;
        heap->map[index + 1].isAllocated = 0;

        //redefine size of this map
        heap->map[index].size = requiredSize;

        //increase map counter
        heap->mapCounter++;
    } else {
        //Next map doesn't exist
        //Create next map
        //Create in between map
        heap->map[index + 1].start = heap->map[index].start + requiredSize;
        heap->map[index + 1].size = heap->map[index].size - requiredSize;
        heap->map[index + 1].isAllocated = 0;

        //redefine size of this map
        heap->map[index].size = requiredSize;

        //increase map counter
        heap->mapCounter++;
    }

    return &(heap->memory[heap->map[index].start]);
}

/**
 * Deallocates allocated memory using memory pointer
 * @param heap  : Pointer to static heap
 * @param ptr   : pointer to allocated memory
 * @return      : 1 for successfully freed (OR) 0 for invalid pointer
 */
static int deallocate(Heap *heap, void *ptr) {
    //search index of map containing information of this pointer
    int index = 0;
    for (; index < heap->mapCounter; ++index)
        if (&heap->memory[heap->map[index].start] == ptr)
            break;// map containing info about this pointer

    if (index >= heap->mapCounter)
        return 0;//map containing info is not found

    //Index is found
    heap->map[index].isAllocated = 0;//free the memory

    //Check the front neighbour if it is free
    int isFrontNeighbourFree = 0;
    if ((index - 1) >= 0)
        isFrontNeighbourFree = !heap->map[index - 1].isAllocated;

    //Check the next neighbour if it is free
    int isNextNeighbourFree = 0;
    if ((index + 1) < heap->mapCounter)
        isNextNeighbourFree = !heap->map[index + 1].isAllocated;

    if (isFrontNeighbourFree && isNextNeighbourFree) {
        //merge next neighbour and this to front neighbour
        heap->map[index - 1].size += heap->map[index].size + heap->map[index + 1].size;
        //shift all to 2 step back
        for (int i = index; i < (heap->mapCounter - 2); ++i)
            heap->map[i] = heap->map[i + 2];

        //decrease map counter
        heap->mapCounter -= 2;
    } else if (isFrontNeighbourFree) {
        //merge this to front neighbour
        heap->map[index - 1].size += heap->map[index].size;
        //shift all to 1 step back from this
        for (int i = index; i < (heap->mapCounter - 1); ++i)
            heap->map[i] = heap->map[i + 1];
        //decrease map counter
        heap->mapCounter--;
    } else if (isNextNeighbourFree) {
        //merge neighbour to this
        heap->map[index].size += heap->map[index + 1].size;
        //shift all to 1 step back after this
        for (int i = index + 1; i < (heap->mapCounter - 1); ++i)
            heap->map[i] = heap->map[i + 1];
        //decrease map counter
        heap->mapCounter--;
    }
    return 1;
}

/**
 * This prints memory mapping of static heap
 * @param heap  : Object of static heap
 */
static void printMemoryMap(Heap heap) {
    for (int i = 0; i < heap.mapCounter; ++i)
        printf("%d : %d, %d, %d\n", i, heap.map[i].start, heap.map[i].size, heap.map[i].isAllocated);
}

/**
 * This prints memory of static heap
 * @param heap  : Object of static heap
 */
static void printMemory(Heap heap) {
    for (int i = 0; i < heap.mapCounter; ++i) {
        int size = heap.map[i].size;
        int start = heap.map[i].start;
        if (heap.map[i].isAllocated) {
            for (int j = 0; j < size; ++j)
                printf("0x%x : x\n", j + start);
        } else {
            for (int j = 0; j < size; ++j)
                printf("0x%x : -\n", j + start);
        }
    }
}

/**
 * This prints both mapping and memory of static heap
 * @param heap  : Object of static heap
 */
static void print(Heap heap) {
    printf("******************\n");
    printMemory(heap);
    printMemoryMap(heap);
}

/**
 * This gives available memory size in bytes
 * @param heap  : Object of static heap
 * @return      : Available Memory in bytes
 */
static int getAvailableBytes(Heap heap){
    int totalSize = 0;
    for (int i = 0; i < heap.mapCounter; ++i)
        if(!heap.map[i].isAllocated)
            totalSize+=heap.map[i].size;
    return totalSize;
}

/**
 * This gives Maximum Individual Available Memory bytes
 * @param heap  : Object of static heap
 * @return      : Maximum Individual Available Memory bytes
 */
static int getAvailableAllocatableBytes(Heap heap){
    int maxIndividualSize = 0;
    for (int i = 0; i < heap.mapCounter; ++i)
        if(!heap.map[i].isAllocated && maxIndividualSize<heap.map[i].size)
            maxIndividualSize = heap.map[i].size;
    return maxIndividualSize;
}

/**
 * This gives available allocation times remaining
 * @param heap  : Object of static heap
 * @return      : Available Size of map
 */
static int getAvailableNumberOfAllocation(Heap heap){
    int allocatedMapSize = 0;
    for (int i = 0; i < heap.mapCounter; ++i)
        if(heap.map[i].isAllocated)
            allocatedMapSize++;
    return heap.mapSize-allocatedMapSize;
}



struct HeapControl StaticHeap = {
        .init = init,
        .malloc = allocate,
        .free = deallocate,
        .printMemory = printMemory,
        .printMemoryMap = printMemoryMap,
        .print = print,
        .getAvailableBytes = getAvailableBytes,
        .getAvailableAllocatableBytes = getAvailableAllocatableBytes,
        .getAvailableNumberOfAllocation = getAvailableNumberOfAllocation
};