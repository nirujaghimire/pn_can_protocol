//
// Created by peter on 6/18/2023.
//

#include "queue.h"

#include <stdio.h>
#include "../test/test.h"

static int testNew() {
    Queue *queue = StaticQueue.new(NULL);
    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__, NULL != queue);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->front == NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->back == NULL);
    check = check && INT_EQUALS(__FILE__, __LINE__, 0, queue->size);

    printf("testNew\n");
    StaticQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());
    return check;
}

static int testEnqueue() {
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(NULL);
    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.enqueue(queue,&values[0])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.enqueue(queue,&values[1])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.enqueue(queue,&values[2])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.enqueue(queue,&values[3])!=NULL);

    double queValues[4];
    queValues[0] = *(double *)queue->front->value;
    queValues[1] = *(double *)queue->front->next->value;
    queValues[2] = *(double *)queue->front->next->next->value;
    queValues[3] = *(double *)queue->front->next->next->next->value;

    check = check && DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__, values, queValues, 4);
    check = check && INT_EQUALS(__FILE__, __LINE__, 4,queue->size);

    printf("testEnqueue\n");
    StaticQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());
    return check;
}

static int testDequeue() {
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(NULL);
    StaticQueue.enqueue(queue,&values[0]);
    StaticQueue.enqueue(queue,&values[1]);
    StaticQueue.enqueue(queue,&values[2]);
    StaticQueue.enqueue(queue,&values[3]);

    double queValues[4];
    queValues[0] = *(double *)StaticQueue.dequeue(queue);
    queValues[1] = *(double *)StaticQueue.dequeue(queue);
    queValues[2] = *(double *)StaticQueue.dequeue(queue);
    queValues[3] = *(double *)StaticQueue.dequeue(queue);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.dequeue(queue)==QUEUE_NULL);
    check = check && DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__, values, queValues, 4);
    check = check && INT_EQUALS(__FILE__, __LINE__, 0,queue->size);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->front==NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->back==NULL);

    printf("testDequeue\n");
    StaticQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());
    return check;
}

static int testPeek() {
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(NULL);
    StaticQueue.enqueue(queue,&values[0]);
    StaticQueue.enqueue(queue,&values[1]);
    StaticQueue.enqueue(queue,&values[2]);
    StaticQueue.enqueue(queue,&values[3]);

    double queValues[4];
    queValues[0] = *(double *)StaticQueue.peek(queue);
    StaticQueue.dequeue(queue);
    queValues[1] = *(double *)StaticQueue.peek(queue);
    StaticQueue.dequeue(queue);
    queValues[2] = *(double *)StaticQueue.peek(queue);
    StaticQueue.dequeue(queue);
    queValues[3] = *(double *)StaticQueue.peek(queue);
    StaticQueue.dequeue(queue);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.peek(queue)==QUEUE_NULL);
    check = check && DOUBLE_ARRAY_EQUALS(__FILE__, __LINE__, values, queValues, 4);

    printf("testPeek\n");
    StaticQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());
    return check;
}

static int testFree() {
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(NULL);
    StaticQueue.enqueue(queue,&values[0]);
    StaticQueue.enqueue(queue,&values[1]);
    StaticQueue.enqueue(queue,&values[2]);
    StaticQueue.enqueue(queue,&values[3]);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.free(&queue)==1);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.free(&queue)==0);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,queue==NULL);

    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());

    printf("testFree\n");
    return check;
}

static int testDoesExist() {
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(NULL);
    StaticQueue.enqueue(queue,&values[0]);
    StaticQueue.enqueue(queue,&values[1]);
    StaticQueue.enqueue(queue,&values[2]);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.doesExist(queue,&values[0]));
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.doesExist(queue,&values[1]));
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticQueue.doesExist(queue,&values[2]));
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,!StaticQueue.doesExist(queue,&values[3]));

    printf("testDoesExist\n");
    StaticQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticQueue.getAllocatedMemories());
    return check;
}


static void test(){
    Test test = StaticTest.new();

    StaticTest.addTask(&test, testNew);
    StaticTest.addTask(&test, testEnqueue);
    StaticTest.addTask(&test, testDequeue);
    StaticTest.addTask(&test, testPeek);
    StaticTest.addTask(&test, testFree);
    StaticTest.addTask(&test, testDoesExist);

    StaticTest.run(&test);
}

static void print(QueueType value){
    printf("%f ",*(double *)value);
}

static void demo(){
    double values[] = {1.2, 3.4, 6.8, 7.9};

    Queue *queue = StaticQueue.new(print);

    StaticQueue.enqueue(queue,&values[0]);
    StaticQueue.print(queue);
    printf("\n\n");

    StaticQueue.enqueue(queue,&values[1]);
    StaticQueue.enqueue(queue,&values[2]);
    StaticQueue.enqueue(queue,&values[3]);
    StaticQueue.print(queue);
    printf("\n\n");

    printf("%f removed\n",*(double *)StaticQueue.dequeue(queue));
    StaticQueue.print(queue);
    printf("\n\n");

    printf("%f is current front element\n",*(double *)StaticQueue.peek(queue));
    StaticQueue.print(queue);
    printf("\n\n");

    StaticQueue.free(&queue);
    StaticQueue.print(queue);
}

//int main(){
//    test();
////    demo();
//
//    return 0;
//}


