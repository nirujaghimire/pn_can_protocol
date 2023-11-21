//
// Created by peter on 9/21/2023.
//

#ifndef CAN_QUEUE_H
#define CAN_QUEUE_H

#include "stdint.h"

#define CAN_DATA_MAX_SIZE 30

typedef struct {
	uint32_t ID;
	uint8_t byte[8];
	uint8_t len;
} CANData;
extern CANData CAN_DATA_NULL;

typedef struct {
	int size;
	int front;
	int back;
	CANData value[CAN_DATA_MAX_SIZE];
} CANQueue;

struct CANQueueControl {
	CANQueue (*new)();
	int (*enqueue)(CANQueue *queue, uint32_t ID, uint8_t *bytes,uint8_t len);
	CANData (*peek)(CANQueue *queue);
	CANData (*dequeue)(CANQueue *queue);
	void (*print)(CANQueue *queue);
};

extern struct CANQueueControl StaticCANQueue;

#endif //CAN_QUEUE_H
