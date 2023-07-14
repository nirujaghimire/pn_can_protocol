//
// Created by peter on 6/18/2023.
//

#ifndef C_LIB_QUEUE_H
#define C_LIB_QUEUE_H


#define QueueType void*
#define QUEUE_NULL NULL

struct QueueData {
    QueueType value;
    struct QueueData *next;
};
typedef struct {
    void (*printEachElement)(QueueType value);
    int size;
    struct QueueData *front;
    struct QueueData *back;
} Queue;

struct QueueControl {
    /**
     * Computation Cost : O(1)\n
     * It allocates the memory for queue and return allocated Queue
     * @printEachElementFunc : Call back function called for each data when print is called
     * @return : Allocated Queue (!!! Must be free using free) (OR) NULL if heap is full
     */
    Queue *(*new)(void (*printEachElementFunc)(QueueType value));

    /**
     * Computation Cost : O(1)\n
     * It adds the element to the end of the queue
     * @param queue     : Queue
     * @param value     : Value to be added in queue
     * @return          : Same que (OR) NULL if heap is full or queue is null
     */
    Queue *(*enqueue)(Queue *queue, QueueType value);

    /**
     * Computation Cost : O(1)\n
     * It remove the element from the front of the queue and return it
     * @param queue     : Queue
     * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
     */
    QueueType (*dequeue)(Queue *queue);

    /**
     * Computation Cost : O(1)\n
     * It returns the element from the front of the queue without removing it
     * @param queue     : Queue
     * @return          : Element in front (OR) QUE_NULL if queue is empty or queue is null
     */
    QueueType(*peek)(Queue *queue);

    /**
     * Computation Cost : O(n)\n
     * It delete all the data and free memories allocated by queue
     * @param queuePtr  : Address of pointer to queue
     * @return          : 1 for success (OR) 0 for failed
     */
    int (*free)(Queue **queuePtr);

    /**
     * Computation Cost : O(n)\n
     * It checks if the value exists in the queue
     * @param queue     : Queue
     * @param value     : Value to be added in queue
     * @return          : 1 if exists (OR) 0 else wise
     */
    int (*doesExist)(Queue *queue, QueueType value);

    /**
     * This will print the contents of que
     * @param queue : Queue to be printed
     */
    void (*print)(Queue *queue);

    /**
     * This return allocated memory for queue till now
     * @return  : Allocated memories
     */
    int (*getAllocatedMemories)();
};

extern struct QueueControl StaticQueue;

#endif //C_LIB_QUEUE_H
