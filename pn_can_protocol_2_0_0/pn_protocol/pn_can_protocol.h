/*
 * pn_can_protocol.h
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */

#ifndef PN_CAN_PROTOCOL_H_
#define PN_CAN_PROTOCOL_H_

#include "pn_can_sync_layer.h"
#include "hash_map.h"
#include "queue.h"
#include "buddy_heap.h"
#include "CAN_queue.h"
#include "stdlib.h"

#define PN_CAN_PROTOCOL_MAX_LINK_SIZE 3
#define PN_CAN_PROTOCOL_NUM_OF_TRY 3

typedef struct {
	SyncLayerCANLink link;
	int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size, int status);
	int (*rxCallback)(uint32_t id, uint8_t *bytes, uint16_t size, int status);
	BuddyHeap *heap;
	HashMap *txMap;
	Queue *txQueue;
	HashMap *rxMap;
	CANQueue txCanQueue;
	CANQueue rxCanQueue;
	int isQueue;
} CANLink;

struct CANLinkControl {
	/**
	 * This will add new link
	 * @param startReqID	: Start request ID
	 * @param startAckID	: Start acknowledge ID
	 * @param endReqID		: End request ID
	 * @param endAckID		: End acknowledge ID
	 * @param canSend		: Function that transmits CAN data
	 * @param txCallback	: Callback that is called after transmit completed
	 * @param rxCallback	: Callback that is called after receive completed
	 * @param isQueue		: 1 for use queue in transmit and 0 for use of map in transmit
	 * @return				: Return the newly created link
	 */
	CANLink* (*newLink)(uint32_t startReqID, uint32_t startAckID,
			uint32_t endReqID, uint32_t endAckID,
			int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
			int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
					int status),
			int (*rxCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
					int status), BuddyHeap *heap, int isQueue);

	/**
	 * This adds message to be transmitted without dynamic memory allocation
	 * @param link	: Pointer to instance of CAN link
	 * @param id	: ID of bytes to be transmitted
	 * @param bytes	: Bytes to be transmitted
	 * @param size	: Size in bytes
	 */
	void (*addTxMsgPtr)(CANLink *link, uint32_t id, uint8_t *bytes,
			uint16_t size);

	/**
	 * This adds message to be transmitted with a dynamic memory allocation
	 * @param link	: Pointer to instance of CAN link
	 * @param id	: ID of bytes to be transmitted
	 * @param bytes	: Bytes to be transmitted
	 * @param size	: Size in bytes
	 */
	void (*addTxMsg)(CANLink *link, uint32_t id, uint8_t *bytes, uint16_t size);

	/**
	 * This adds message to be received without dynamic memory allocation
	 * @param link	: Pointer to instance if CAN link
	 * @param id	: ID of bytes to be received
	 * @param bytes	: Bytes to be received
	 * @param size	: Size in bytes
	 */
	void (*addRxMsgPtr)(CANLink *link, uint32_t id, uint8_t *bytes,
			uint16_t size);

	/**
	 * This should be called continuously in while loop
	 * @param link	: Pointer to instance of CAN link
	 */
	void (*thread)(CANLink *link);

	/**
	 * This should be called when CAN message is received
	 * @param link	: Pointer to instance if CAN link
	 * @param id	: ID of bytes received
	 * @param bytes	: Bytes received
	 */
	void (*canReceive)(CANLink *link, uint32_t id, uint8_t *bytes, uint16_t len);
};
extern struct CANLinkControl StaticCANLink;

#endif /* PN_CAN_PROTOCOL_H_ */
