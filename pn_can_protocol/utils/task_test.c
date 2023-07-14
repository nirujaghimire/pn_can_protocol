//
// Created by peter on 6/21/2023.
//

#include "task.h"

#include <stdio.h>
#include <stdint.h>
#include "../test/test.h"


int len = 3;
static int task0(){
    static int count = 0;
    printf("Task0: %d\n",count);
    count++;
    if(count<len)
        return TASK_BUSY;
    return TASK_SUCCESS;
}
static int task1(){
    static int count = 0;
    printf("Task1: %d\n",count);
    count++;
    if(count<len)
        return TASK_BUSY;
    return TASK_SUCCESS;
}
static int task2(){
    static int count = 0;
    printf("Task2: %d\n",count);
    count++;
    if(count<len)
        return TASK_BUSY;
    return TASK_SUCCESS;
}
static int task3(){
    static int count = 0;
    printf("Task3: %d\n",count);
    count++;
    if(count<len)
        return TASK_BUSY;
    return TASK_SUCCESS;
}

static int testNew() {
    TaskQueue *queue = StaticTaskQueue.new(NULL);
    int check = BOOLEAN_IS_TRUE(__FILE__, __LINE__, NULL != queue);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->front == NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->back == NULL);
    check = check && INT_EQUALS(__FILE__, __LINE__, 0, queue->size);

    printf("testNew\n");
    StaticTaskQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticTaskQueue.getAllocatedMemories());
    return check;
}

static int testEnqueue() {
    Task values[] = {task0,task1,task2,task3};

    TaskQueue *queue = StaticTaskQueue.new(NULL);
    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.enqueue(queue,values[0])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.enqueue(queue,values[1])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.enqueue(queue,values[2])!=NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.enqueue(queue,values[3])!=NULL);

    Task queValues[4];
    queValues[0] = queue->front->task;
    queValues[1] = queue->front->next->task;
    queValues[2] = queue->front->next->next->task;
    queValues[3] = queue->front->next->next->next->task;

    for(int i=0;i<4;i++)
        check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, values[i] == queValues[i]);
    check = check && INT_EQUALS(__FILE__, __LINE__, 4,queue->size);

    printf("testEnqueue\n");
    StaticTaskQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticTaskQueue.getAllocatedMemories());
    return check;
}

static int testDequeue() {
    Task values[] = {task0,task1,task2,task3};

    TaskQueue *queue = StaticTaskQueue.new(NULL);
    StaticTaskQueue.enqueue(queue,values[0]);
    StaticTaskQueue.enqueue(queue,values[1]);
    StaticTaskQueue.enqueue(queue,values[2]);
    StaticTaskQueue.enqueue(queue,values[3]);

    Task queValues[4];
    queValues[0] = StaticTaskQueue.dequeue(queue);
    queValues[1] = StaticTaskQueue.dequeue(queue);
    queValues[2] = StaticTaskQueue.dequeue(queue);
    queValues[3] = StaticTaskQueue.dequeue(queue);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.dequeue(queue)==TASK_NULL);
    for(int i=0;i<4;i++)
        check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, values[i] == queValues[i]);
    check = check && INT_EQUALS(__FILE__, __LINE__, 0,queue->size);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->front==NULL);
    check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, queue->back==NULL);

    printf("testDequeue\n");
    StaticTaskQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticTaskQueue.getAllocatedMemories());
    return check;
}

static int testPeek() {
    Task values[] = {task0,task1,task2,task3};

    TaskQueue *queue = StaticTaskQueue.new(NULL);
    StaticTaskQueue.enqueue(queue,values[0]);
    StaticTaskQueue.enqueue(queue,values[1]);
    StaticTaskQueue.enqueue(queue,values[2]);
    StaticTaskQueue.enqueue(queue,values[3]);

    Task queValues[4];
    queValues[0] = StaticTaskQueue.peek(queue);
    StaticTaskQueue.dequeue(queue);
    queValues[1] = StaticTaskQueue.peek(queue);
    StaticTaskQueue.dequeue(queue);
    queValues[2] = StaticTaskQueue.peek(queue);
    StaticTaskQueue.dequeue(queue);
    queValues[3] = StaticTaskQueue.peek(queue);
    StaticTaskQueue.dequeue(queue);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.peek(queue)==TASK_NULL);
    for(int i=0;i<4;i++)
        check = check && BOOLEAN_IS_TRUE(__FILE__, __LINE__, values[i] == queValues[i]);

    printf("testPeek\n");
    StaticTaskQueue.free(&queue);
    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticTaskQueue.getAllocatedMemories());
    return check;
}

static int testFree() {
    Task values[] = {task0,task1,task2,task3};

    TaskQueue *queue = StaticTaskQueue.new(NULL);
    StaticTaskQueue.enqueue(queue,values[0]);
    StaticTaskQueue.enqueue(queue,values[1]);
    StaticTaskQueue.enqueue(queue,values[2]);
    StaticTaskQueue.enqueue(queue,values[3]);

    int check = BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.free(&queue)==1);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,StaticTaskQueue.free(&queue)==0);
    check = check && BOOLEAN_IS_TRUE(__FILE__,__LINE__,queue==NULL);

    check = check && INT_EQUALS(__FILE__, __LINE__,0,StaticTaskQueue.getAllocatedMemories());

    printf("testFree\n");
    return check;
}

static void test(){
    Test test = StaticTest.new();

    StaticTest.addTask(&test, testNew);
    StaticTest.addTask(&test, testEnqueue);
    StaticTest.addTask(&test, testDequeue);
    StaticTest.addTask(&test, testPeek);
    StaticTest.addTask(&test, testFree);

    StaticTest.run(&test);
}

static void print(Task task){
    printf("%p ",task);
}

static void demo(){
    Task values[] = {task0,task1,task2,task3};

    TaskQueue *queue = StaticTaskQueue.new(print);
    StaticTaskQueue.enqueue(queue,values[0]);
    StaticTaskQueue.enqueue(queue,values[1]);
    StaticTaskQueue.enqueue(queue,values[2]);
    StaticTaskQueue.enqueue(queue,values[3]);

    printf("Tasks : ");
    StaticTaskQueue.print(queue);
    printf("\n\n");

    for (int i = 0; i < 1000; ++i) {
        StaticTaskQueue.execute(queue);
        if(queue->size==0)
            break;
    }
    printf("\n");

    printf("Tasks : ");
    StaticTaskQueue.print(queue);
    printf("\n\n");

    StaticTaskQueue.free(&queue);

    printf("Tasks : ");
    StaticTaskQueue.print(queue);
}

//int main(){
////    test();
//    demo();
//
//    return 0;
//}


