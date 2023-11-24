//
// Created by peter on 9/21/2023.
//

#include "CAN_queue.h"
#include "stdio.h"

typedef CANQueue Queue;
typedef CANData Value;

Value CAN_DATA_NULL = { 0xFFFFFFFF, { 0, 0, 0, 0, 0, 0, 0, 0 } };

static Queue new() {
	Queue queue;
	queue.front = 0;
	queue.back = 0;
	queue.size = 0;
	for (int i = 0; i < CAN_DATA_MAX_SIZE; i++)
		queue.value[i] = CAN_DATA_NULL;
	return queue;
}

static int enqueue(Queue *queue, uint32_t ID, uint8_t *bytes,uint8_t size) {
	if (queue->size >= CAN_DATA_MAX_SIZE)
		return 0;

	Value value = { .ID = ID ,.len=size};
	*(uint64_t*) value.byte = *(uint64_t*) bytes;

	queue->value[queue->back] = value;
	queue->back++;
	queue->back %= CAN_DATA_MAX_SIZE;

	queue->size++;
	return 1;
}

static Value peek(Queue *queue) {
	return queue->value[queue->front];
}

static Value dequeue(Queue *queue) {
	if (queue->size <= 0)
		return CAN_DATA_NULL;

	Value value = queue->value[queue->front];
	queue->value[queue->front] = CAN_DATA_NULL;

	queue->size--;

	queue->front++;
	queue->front %= CAN_DATA_MAX_SIZE;
	return value;
}

static void print(Queue *queue) {
	printf("%d-%d => [", queue->front, queue->back);
	for (int i = 0; i < CAN_DATA_MAX_SIZE; ++i) {
		if (queue->value[i].ID == 0xFFFFFFFF)
			printf("[_]");
		else
			printf("[0x%x]", (int) queue->value[i].ID);
	}
	printf("]\n");
}

struct CANQueueControl StaticCANQueue = { .new = new, .enqueue = enqueue, .peek = peek, .dequeue = dequeue, .print = print };

