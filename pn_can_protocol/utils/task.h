//
// Created by peter on 6/21/2023.
//

#ifndef C_LIB_TASK_H
#define C_LIB_TASK_H

#define TASK_NULL NULL

typedef enum {
    TASK_SUCCESS = 1,
    TASK_BUSY = 0
}TaskStatus;

typedef int (*Task)();

struct TaskQueueData {
    Task task;
    struct TaskQueueData *next;
};
typedef struct {
    void (*printEachElement)(Task value);
    int size;
    struct TaskQueueData *front;
    struct TaskQueueData *back;
} TaskQueue;

struct TaskQueControl {
    /**
     * Computation Cost : O(1)\n
     * It allocates the memory for queue and return allocated TaskQueue
     * @printEachElementFunc : Call back function called for each data when print is called
     * @return : Allocated TaskQueue (!!! Must be free using free) (OR) NULL if heap is full
     */
    TaskQueue *(*new)(void (*printEachElementFunc)(Task value));

    /**
     * Computation Cost : O(1)\n
     * It adds the element to the end of the queue
     * @param queue     : TaskQueue
     * @param value     : Value to be added in queue
     * @return          : Same que (OR) NULL if heap is full or queue is null
     */
    TaskQueue *(*enqueue)(TaskQueue *queue, Task value);

    /**
     * Computation Cost : O(1)\n
     * It remove the element from the front of the queue and return it
     * @param queue     : TaskQueue
     * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
     */
    Task (*dequeue)(TaskQueue *queue);

    /**
     * Computation Cost : O(1)\n
     * It returns the element from the front of the queue without removing it
     * @param queue     : TaskQueue
     * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
     */
    Task(*peek)(TaskQueue *queue);

    /**
     * Computation Cost : O(n)\n
     * It delete all the data and free memories allocated by queue
     * @param queuePtr  : Address of pointer to queue
     * @return          : 1 for success (OR) 0 for failed
     */
    int (*free)(TaskQueue **queuePtr);

    /**
     * This will print the contents of que
     * @param queue : TaskQueue to be printed
     */
    void (*print)(TaskQueue *queue);

    /**
     * This should be called regularly. It handles all the tasks and run it.
     * @param queue     : TaskQue
     */
    void (*execute)(TaskQueue *queue);

    /**
     * This return allocated memory for queue till now
     * @return  : Allocated memories
     */
    int (*getAllocatedMemories)();

};

extern struct TaskQueControl StaticTaskQueue;
#endif //C_LIB_TASK_H
