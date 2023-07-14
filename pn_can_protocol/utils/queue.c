//
// Created by peter on 6/18/2023.
//

#include "queue.h"

#include <malloc.h>

#define MAX_LOOP 1000

static int allocatedMemory = 0;

typedef struct QueueData QueueData;

/**
 * It allocates the memory and return pointer to it
 * @param sideInByte    : Size in bytes
 * @return              : Pointer to allocated memory
 *                      : NULL if there exist no memory for allocation
 */
static void *allocateMemory(int sideInByte) {
    void* ptr = malloc(sideInByte);
    if(ptr!=NULL)
        allocatedMemory+=sideInByte;
    return ptr;
}

/**
 * It free the allocated memory
 * @param pointer       : Pointer to allocated Memory
 * @param sizeInByte    : Size to be freed
 * @return              : 1 for success (OR) 0 for failed
 */
static int freeMemory(void *pointer, int sizeInByte) {
    free(pointer);
    allocatedMemory-=sizeInByte;
    return 1;
}

/**
 * Computation Cost : O(1)\n
 * It allocates the memory for queue and return allocated Queue
 * @printEachElementFunc : Call back function called for each data when print is called
 * @return : Allocated Queue (!!! Must be free using free) (OR) NULL if heap is full
 */
static Queue *new(void (*printEachElementFunc)(QueueType value)) {
    //Allocate memory for hash map
    Queue *queue = allocateMemory(sizeof(Queue));

    //Heap is full
    if (queue == NULL)
        return NULL;

    queue->printEachElement=printEachElementFunc;

    //Making both front and back null
    queue->front = NULL;
    queue->back = NULL;

    //Make initial size zero
    queue->size = 0;
    return queue;
}

/**
 * Computation Cost : O(1)\n
 * It adds the element to the end of the queue
 * @param queue     : Queue
 * @param value     : Value to be added in queue
 * @return          : Same que (OR) NULL if heap is full or queue is null
 */
static Queue *enqueue(Queue *queue, QueueType value) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return NULL;

    //Allocate Memory for newData
    QueueData *newData = allocateMemory(sizeof(QueueData));

    //If heap is full then return NULL
    if (newData == NULL)
        return NULL;

    //Fill the value in data
    newData->value = value;
    newData->next = NULL;//make next data is empty

    //Get the last data
    if (queue->size == 0) {
        //If que is empty
        queue->front = newData;
        queue->back = newData;
    } else {
        queue->back->next = newData;
        queue->back = newData;
    }
    //If data is added then increase the size
    queue->size++;

    //Return same @queue
    return queue;
}

/**
 * Computation Cost : O(1)\n
 * It remove the element from the front of the queue and return it
 * @param queue     : Queue
 * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
 */
static QueueType dequeue(Queue *queue) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return QUEUE_NULL;

    //Get the last data
    if (queue->size == 0) {
        //If que is empty
        return QUEUE_NULL;
    } else {
        //Get the front data of queue
        QueueData *frontData = queue->front;
        QueueType value = frontData->value;

        //Put front second data in the front
        queue->front = frontData->next;

        //Deallocate the allocated memory by front data
        freeMemory(frontData, sizeof(QueueData));

        //Decrease the size of queue
        queue->size--;
        if(queue->size==0) {
            queue->front = NULL;
            queue->back = NULL;
        }
        return value;
    }
}

/**
 * Computation Cost : O(1)\n
 * It returns the element from the front of the queue without removing it
 * @param queue     : Queue
 * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
 */
static QueueType peek(Queue *queue) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return QUEUE_NULL;

    if (queue->size == 0) {
        //If que is empty
        return QUEUE_NULL;
    } else {
        //Return the front element of queue
        return queue->front->value;
    }
}

/**
 * Computation Cost : O(n)\n
 * It delete all the data and free memories allocated by queue
 * @param queuePtr  : Address of pointer to queue
 * @return          : 1 for success (OR) 0 for failed
 */
static int freeQue(Queue **queuePtr) {
    Queue *queue = *queuePtr;
    //If queue is NULL
    if (queue == NULL)
        return 0;

    int size = queue->size;
    //If queue is empty
    if (size == 0) {
        //Free hash map memory
        freeMemory(queue, sizeof(Queue));
        *queuePtr = NULL;
        return 1;
    }

    //Delete all data
    for (int i = 0; i < size; ++i)
        dequeue(queue);

    //Free memory for queue
    freeMemory(queue, sizeof(Queue));

    *queuePtr = NULL;

    return 1; //freeing memory success
}

/**
 * Computation Cost : O(n)\n
 * It checks if the value exists in the queue
 * @param queue     : Queue
 * @param value     : Value to be added in queue
 * @return          : 1 if exists (OR) 0 else wise
 */
static int doesExist(Queue *queue, QueueType value){
    if(value==QUEUE_NULL)
        return 0;
    //If map is NULL then return NULL
    if (queue == NULL)
        return 0;

    if (queue->size == 0) {
        //If que is empty
        return 0;
    } else {
        QueueData *data = queue->front;
        for (int i = 0; i <queue->size; ++i) {
            if(data->value==value)
                return 1;
            data=data->next;
        }
        return 0;
    }
}

/**
 * This will print the contents of que
 * @param queue : Queue to be printed
 */
static void print(Queue *queue){
    if(queue==NULL)
        return;

    if(queue->printEachElement==NULL)
        return;

    QueueData* data = queue->front;
    for (int i = 0; i < queue->size; ++i) {
        if(data==NULL)
            break;
        queue->printEachElement(data->value);
        data=data->next;
    }
}

/**
 * This return allocated memory for queue till now
 * @return  : Allocated memories
 */
static int getAllocatedMemories(){
    return allocatedMemory;
}


struct QueueControl StaticQueue = {
        .new=new,
        .enqueue = enqueue,
        .dequeue = dequeue,
        .peek = peek,
        .free=freeQue,
        .doesExist=doesExist,
        .print=print,
        .getAllocatedMemories=getAllocatedMemories
};

