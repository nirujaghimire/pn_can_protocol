//
// Created by niruja on 9/23/2023.
//

#ifndef C_LIB_STATIC_HEAP_H
#define C_LIB_STATIC_HEAP_H
#include "stdint.h"

typedef uint8_t HeapMemory;
typedef struct {
    uint16_t start;
    uint16_t size;
    uint8_t isAllocated;
}HeapMemoryMap;
typedef struct{
    HeapMemoryMap* map;
    HeapMemory* memory;
    int mapSize;
    int memorySize;
    int mapCounter;
}Heap;

struct HeapControl{
    /**
     * This initiates the static heap
     * @param heap          : Pointer to static heap
     * @param map           : Pointer to statically allocated map
     * @param mapSize       : Max number of allocation
     * @param memory        : Pointer to statically allocated memory
     * @param memorySize    : Max size of memory
     */
    void (*init)(Heap *heap, HeapMemoryMap *map, int mapSize, HeapMemory *memory, int memorySize);

    /**
     * Allocates memory of required size
     * @param heap          : Pointer to static heap
     * @param requiredSize  : Required size in byte
     * @return              : pointer to allocated memory (OR)
     *                          NULL if memory is not available
     */
    void* (*malloc)(Heap *heap, int requiredSize);

    /**
     * Deallocates allocated memory using memory pointer
     * @param heap  : Pointer to static heap
     * @param ptr   : pointer to allocated memory
     * @return      : 1 for successfully freed (OR) 0 for invalid pointer
     */
    int (*free)(Heap *heap, void *ptr);

    /**
     * This prints memory mapping of static heap
     * @param heap  : Object of static heap
     */
    void (*printMemoryMap)(Heap heap);

    /**
     * This prints memory of static heap
     * @param heap  : Object of static heap
     */
    void (*printMemory)(Heap heap);

    /**
     * This gives available memory size in bytes
     * @param heap  : Object of static heap
     * @return      : Available Memory in bytes
     */
    int (*getAvailableBytes)(Heap heap);

    /**
     * This gives Maximum Individual Available Memory bytes
     * @param heap  : Object of static heap
     * @return      : Maximum Individual Available Memory bytes
     */
    int (*getAvailableAllocatableBytes)(Heap heap);

    /**
     * This number of available allocation
     * @param heap  : Object of static heap
     * @return      : Available Size of map
     */
    int (*getAvailableNumberOfAllocation)(Heap heap);

    /**
     * This prints both mapping and memory of static heap
     * @param heap  : Object of static heap
     */
    void (*print)(Heap heap);
};

extern struct HeapControl StaticHeap;
#endif //C_LIB_STATIC_HEAP_H
