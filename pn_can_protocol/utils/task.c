//
// Created by peter on 6/21/2023.
//

#include "task.h"

#include <malloc.h>

static int allocatedMemory = 0;

typedef struct TaskQueueData TaskQueueData;

/**
 * It allocates the memory and return pointer to it
 * @param sideInByte    : Size in bytes
 * @return              : Pointer to allocated memory
 *                      : NULL if there exist no memory for allocation
 */
static void *allocateMemory(int sideInByte) {
    void *ptr = malloc(sideInByte);
    if (ptr != NULL)
        allocatedMemory += sideInByte;
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
    allocatedMemory -= sizeInByte;
    return 1;
}

/**
 * Computation Cost : O(1)\n
 * It allocates the memory for queue and return allocated TaskQueue
 * @printEachElementFunc : Call back function called for each data when print is called
 * @return : Allocated TaskQueue (!!! Must be free using free) (OR) NULL if heap is full
 */
static TaskQueue *new(void (*printEachElementFunc)(Task task)) {
    //Allocate memory for hash map
    TaskQueue *queue = allocateMemory(sizeof(TaskQueue));

    //Heap is full
    if (queue == NULL)
        return NULL;

    queue->printEachElement = printEachElementFunc;

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
 * @param queue     : TaskQueue
 * @param task      : Task to be added in queue
 * @return          : Same que (OR) NULL if heap is full or queue is null
 */
static TaskQueue *enqueue(TaskQueue *queue, Task task) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return NULL;

    //Allocate Memory for newData
    TaskQueueData *newData = allocateMemory(sizeof(TaskQueueData));

    //If heap is full then return NULL
    if (newData == NULL)
        return NULL;

    //Fill the task in data
    newData->task = task;
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
 * @param queue     : TaskQueue
 * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
 */
static Task dequeue(TaskQueue *queue) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return TASK_NULL;

    //Get the last data
    if (queue->size == 0) {
        //If que is empty
        return TASK_NULL;
    } else {
        //Get the front data of queue
        TaskQueueData *frontData = queue->front;
        Task task = frontData->task;

        //Put front second data in the front
        queue->front = frontData->next;

        //Deallocate the allocated memory by front data
        freeMemory(frontData, sizeof(TaskQueueData));

        //Decrease the size of queue
        queue->size--;
        if (queue->size == 0) {
            queue->front = NULL;
            queue->back = NULL;
        }
        return task;
    }
}

/**
 * Computation Cost : O(1)\n
 * It returns the element from the front of the queue without removing it
 * @param queue     : TaskQueue
 * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
 */
static Task peek(TaskQueue *queue) {
    //If map is NULL then return NULL
    if (queue == NULL)
        return TASK_NULL;

    if (queue->size == 0) {
        //If que is empty
        return TASK_NULL;
    } else {
        //Return the front element of queue
        return queue->front->task;
    }
}

/**
 * Computation Cost : O(n)\n
 * It delete all the data and free memories allocated by queue
 * @param queuePtr  : Address of pointer to queue
 * @return          : 1 for success (OR) 0 for failed
 */
static int freeQue(TaskQueue **queuePtr) {
    TaskQueue *queue = *queuePtr;
    //If queue is NULL
    if (queue == NULL)
        return 0;

    int size = queue->size;
    //If queue is empty
    if (size == 0) {
        //Free hash map memory
        freeMemory(queue, sizeof(TaskQueue));
        *queuePtr = NULL;
        return 1;
    }

    //Delete all data
    for (int i = 0; i < size; ++i)
        dequeue(queue);

    //Free memory for queue
    freeMemory(queue, sizeof(TaskQueue));

    *queuePtr = NULL;

    return 1; //freeing memory success
}

/**
 * This will print the contents of que
 * @param queue : TaskQueue to be printed
 */
static void print(TaskQueue *queue) {
    if (queue == NULL)
        return;

    if (queue->printEachElement == NULL)
        return;

    TaskQueueData *data = queue->front;
    for (int i = 0; i < queue->size; ++i) {
        if (data == NULL)
            break;
        queue->printEachElement(data->task);
        data = data->next;
    }
}

/**
 * This should be called regularly. It handles all the tasks and run it.
 * @param queue     : TaskQue
 */
static void execute(TaskQueue *queue){
    //If queue is NULL
    if (queue == NULL)
        return;

    int size = queue->size;
    //If queue is empty
    if (size == 0)
        return;

    Task task = dequeue(queue);
    if(task == TASK_NULL)//If task is corrupt
        return;

    //If task is not corrupt run it
    TaskStatus status = task();

    //If task is busy then again push the task at the end
    if(status==TASK_BUSY)
        enqueue(queue, task);
}

/**
 * This return allocated memory for queue till now
 * @return  : Allocated memories
 */
static int getAllocatedMemories() {
    return allocatedMemory;
}
struct TaskQueControl StaticTaskQueue = {
        .new=new,
        .enqueue = enqueue,
        .dequeue = dequeue,
        .peek = peek,
        .free=freeQue,
        .print=print,
        .execute = execute,
        .getAllocatedMemories=getAllocatedMemories
};



