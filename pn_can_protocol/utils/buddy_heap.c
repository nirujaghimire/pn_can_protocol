//
// Created by niruja on 10/8/2023.
//

#include "buddy_heap.h"
#include "stdio.h"

typedef uint32_t AddrType;

static int get2power(int exponent) {
	return (1 << exponent);
}

static int getLog2(int size) {
	int exponent = 0;
	for (; get2power(exponent) < size; exponent++);
	return exponent;
}

static void setMap(uint8_t *map, int mapIdx, int value) {
	int mainIdx = mapIdx / 8;
	int bitIdx = mapIdx % 8;
	if (value)
		map[mainIdx] |= (1 << bitIdx);
	else
		map[mainIdx] &= ~(1 << bitIdx);
}

static int getMap(const uint8_t *map, int mapIdx) {
	int mainIdx = mapIdx / 8;
	int bitIdx = mapIdx % 8;
	return (map[mainIdx] & (1 << bitIdx)) != 0;
}

/**
 * It checks whether given pointer belongs to this heap
 * @param heap      : Pointer to buddy heap instance
 * @param ptr       : Pointer to the allocated memory
 * @return          : 1 ptr belongs to this heap
 *                  : 0 ptr doesn't belongs to this heap
 */
static int isValidPointer(BuddyHeap heap, void *ptr) {
	if (((AddrType) ptr < (AddrType) (heap.memory)) || ((AddrType) ptr >= ((AddrType) (heap.memory)) + heap.maxSize))
		return 0;
	return 1;
}

static void printRecursion(BuddyHeap heap, int origin, int level, int blockSize) {
	int mapIdx = get2power(level) - 1 + origin / blockSize;
	if (getMap(heap.map, mapIdx)) {
		for (int i = 0; i < blockSize / heap.minSize; ++i)
			printf("0x%x : %d\n", origin + i * heap.minSize, heap.memory[origin + i * heap.minSize]);
		return;
	} else if (level == (heap.nMax - heap.nMin)) {
		printf("0x%x : _\n", origin);
		return;
	}

	if (blockSize <= heap.minSize)
		return;

	printRecursion(heap, origin, level + 1, blockSize / 2);
	printRecursion(heap, origin + blockSize / 2, level + 1, blockSize / 2);
}

/**
 * This prints the memory
 * @param heap      : Pointer to buddy heap instance
 */
static void printMemory(BuddyHeap heap) {
	printRecursion(heap, 0, 0, heap.maxSize);
}

/**
 * This prints buddy heap mapper structure
 * @param heap      : Pointer to buddy heap instance
 */
static void printMap(BuddyHeap heap) {
	for (int level = 0; level < heap.nMax + 1 - heap.nMin; ++level) {
		int len = get2power(level);
		int mapIdx = len - 1;
		int offset = get2power(heap.nMax - heap.nMin - level) - 1;
		int diff = get2power(heap.nMax + 1 - heap.nMin - level) - 1;

		printf("%4d : ", heap.maxSize / get2power(level));
		for (int j = 0; j < offset; ++j)
			printf(" ");
		for (int j = 0; j < len; ++j) {
			printf("%d", getMap(heap.map, mapIdx + j));
			for (int i = 0; i < diff; ++i)
				printf(" ");
		}

		if (level == heap.nMax - heap.nMin) {
			printf("\n");
			break;
		}

		printf("\n       ");
		for (int j = 0; j < offset - 1; ++j)
			printf(" ");
		for (int j = 0; j < len; ++j) {
			printf("/ \\");
			for (int i = 0; i < diff - 2; ++i)
				printf(" ");
		}
		printf("\n");
	}
}

/**
 * This prints both map and memory of buddy heap
 * @param heap      : Pointer to buddy heap instance
 */
static void print(BuddyHeap heap) {
	printf("******************\n");
	printMemory(heap);
	printMap(heap);
}

static int allocateRecursion(BuddyHeap *heap, int origin, int level, int blockSize, int requiredSize, int updateMap) {
	int mapIdx = get2power(level) - 1 + origin / blockSize;
	if (getMap(heap->map, mapIdx) || blockSize < requiredSize)
		return -1;
	else if (blockSize == heap->minSize) {
		if (updateMap)
			setMap(heap->map, mapIdx, 1); //map[mapIdx]=1;
		return origin;
	} else if (blockSize == requiredSize) {
		int leftIndex = allocateRecursion(heap, origin, level + 1, blockSize / 2, blockSize / 2, 0);
		int rightIndex = allocateRecursion(heap, origin + blockSize / 2, level + 1, blockSize / 2, blockSize / 2, 0);
		if (leftIndex == -1 || rightIndex == -1)
			return -1;
		if (updateMap)
			setMap(heap->map, mapIdx, 1); //map[mapIdx]=1;
		return origin;
	} else {
		int leftIndex = allocateRecursion(heap, origin, level + 1, blockSize / 2, requiredSize, updateMap);
		if (leftIndex >= 0)
			return leftIndex;
		int rightIndex = allocateRecursion(heap, origin + blockSize / 2, level + 1, blockSize / 2, requiredSize, updateMap);
		return rightIndex;
	}
}

/**
 * This allocates the heap memory
 * @param heap      : Pointer to buddy heap instance
 * @param size      : Size to be allocated
 * @return          : Pointer to the allocated memory
 *                  : NULL if heap memory is not available
 */
static void* allocate(BuddyHeap *heap, int size) {
	int exponent = getLog2(size);
	int requiredSize = get2power(exponent);
	int memIdx = allocateRecursion(heap, 0, 0, heap->maxSize, requiredSize, 1);
	if (memIdx == -1)
		return NULL;
	return &(heap->memory[memIdx]);
}

static int deallocateRecursion(BuddyHeap *heap, int origin, int level, int blockSize, int requiredMemIdx) {
	int mapIdx = get2power(level) - 1 + origin / blockSize;
	if (getMap(heap->map, mapIdx) == 1 && origin == requiredMemIdx) {
		setMap(heap->map, mapIdx, 0);
		return mapIdx;
	}
	if (blockSize == heap->minSize) {
		return -1;
	} else {
		int leftIndex = deallocateRecursion(heap, origin, level + 1, blockSize / 2, requiredMemIdx);
		if (leftIndex >= 0)
			return leftIndex;
		int rightIndex = deallocateRecursion(heap, origin + blockSize / 2, level + 1, blockSize / 2, requiredMemIdx);
		return rightIndex;
	}
}

/**
 * This deallocates the allocated heap memory
 * @param heap      : Pointer to buddy heap instance
 * @param ptr       : Pointer to the allocated memory
 * @return          : 1 if successfully deallocated
 *                  : 0 if ptr is not valid address or not belongs to this heap
 */
static int deallocate(BuddyHeap *heap, void *ptr) {
	if (!isValidPointer(*heap, ptr))
		return 0;

	int memIdx = (int) ((AddrType) ptr - (AddrType) (heap->memory));
	int mapIdx = deallocateRecursion(heap, 0, 0, heap->maxSize, memIdx);
	return mapIdx > 0;
}

/**
 * This will initiates and return the heap
 * Optimum size of buffer is given by
 * size = (max_memory_size + mapSize(max_memory_size,min_memory_size))
 * @param buffer        : Buffer for memory and map
 * @param bufferSize    : Size of buffer given in bytes
 * @param minSize       : Least size of memory that gets allocated (If 2bytes is minimum size then allocation of 1byte takes 2bytes)
 * @return              : Initiated Buddy heap
 */
static BuddyHeap new(uint8_t *buffer, int bufferSize, int minSize) {
	int maxSize = (bufferSize * 4 * minSize) / (1 + 4 * minSize);
	int nMax = getLog2(maxSize);
	if (get2power(nMax) > maxSize)
		nMax = nMax - 1;
	maxSize = get2power(nMax);
	int mapSize = mapSize(maxSize, minSize);

	BuddyHeap heap;
	heap.memory = buffer;
	heap.map = &buffer[maxSize];
	heap.maxSize = maxSize;
	heap.minSize = minSize;
	heap.mapSize = mapSize;
	heap.nMax = nMax;
	heap.nMin = getLog2(minSize);
	return heap;
}

struct BuddyHeapControl StaticBuddyHeap = { .new = new, .malloc = allocate, .free = deallocate, .isValidPointer = isValidPointer, .printMemory = printMemory, .printMap = printMap, .print = print };
