//
// Created by niruja on 10/8/2023.
//

#ifndef STATIC_HEAP_MEMORY_BUDDY_HEAP_H
#define STATIC_HEAP_MEMORY_BUDDY_HEAP_H
#include "stdint.h"

#define mapSize(memorySize,minMemorySize) ((memorySize*2/minMemorySize)/8)

typedef struct {
    uint8_t *memory;
    uint8_t *map;
    int nMax;
    int nMin;
    int maxSize;
    int minSize;
    int mapSize;
}BuddyHeap;

struct BuddyHeapControl{
    /**
     * This will initiates and return the heap
     * Optimum size of buffer is given by
     * size = (max_memory_size + mapSize(max_memory_size,min_memory_size))
     * @param buffer        : Buffer for memory and map
     * @param bufferSize    : Size of buffer given in bytes
     * @param minSize       : Least size of memory that gets allocated (If 2bytes is minimum size then allocation of 1byte takes 2bytes)
     * @return              : Initiated Buddy heap
     */
    BuddyHeap (*new)(uint8_t *buffer,int bufferSize,int minSize);

    /**
     * This allocates the heap memory
     * @param heap      : Pointer to buddy heap instance
     * @param size      : Size to be allocated
     * @return          : Pointer to the allocated memory
     *                  : NULL if heap memory is not available
     */
    void* (*malloc)(BuddyHeap *heap,int size);

    /**
     * This deallocates the allocated heap memory
     * @param heap      : Pointer to buddy heap instance
     * @param ptr       : Pointer to the allocated memory
     * @return          : 1 if successfully deallocated
     *                  : 0 if ptr is not valid address or not belongs to this heap
     */
    int (*free)(BuddyHeap *heap,void *ptr);

    /**
     * This prints the memory
     * @param heap      : Pointer to buddy heap instance
     */
    void (*printMemory)(BuddyHeap heap);

    /**
     * This prints buddy heap mapper structure
     * @param heap      : Pointer to buddy heap instance
     */
    void (*printMap)(BuddyHeap heap);

    /**
     * This prints both map and memory of buddy heap
     * @param heap      : Pointer to buddy heap instance
     */
    void (*print)(BuddyHeap heap);

    /**
     * It checks whether given pointer belongs to this heap
     * @param heap      : Pointer to buddy heap instance
     * @param ptr       : Pointer to the allocated memory
     * @return          : 1 ptr belongs to this heap
     *                  : 0 ptr doesn't belongs to this heap
     */
    int (*isValidPointer)(BuddyHeap heap,void *ptr);
};


extern struct BuddyHeapControl StaticBuddyHeap;

#endif //STATIC_HEAP_MEMORY_BUDDY_HEAP_H
