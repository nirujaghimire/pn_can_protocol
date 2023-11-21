/*
 * pn_can_protocol.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "pn_can_protocol.h"

//#define CONSOLE_ENABLE

#ifdef CONSOLE_ENABLE
#define TAG "PN_SYNC"
typedef enum {
	CONSOLE_ERROR, CONSOLE_INFO, CONSOLE_WARNING
} ConsoleStatus;
static void console(ConsoleStatus status, const char *func_name,
		const char *msg, ...) {
	printf("%s|%s>", TAG, func_name);
	if (status == CONSOLE_ERROR) {
		printf("ERROR :");
	} else if (status == CONSOLE_INFO) {
		printf("INFO : ");
	} else if (status == CONSOLE_WARNING) {
		printf("WARNING : ");
	} else {
		printf(": ");
	}
	va_list args;
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
}
#endif

static CANLink link[10];
static int linkCount = 0;

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
static CANLink* new(uint32_t startReqID, uint32_t startAckID,
		uint32_t endReqID, uint32_t endAckID,
		int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
		int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
				int status),
		int (*rxCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
				int status), BuddyHeap *heap, int isQueue) {
	int n = linkCount++;
	SyncLayerCANLink syncLink = { .startReqID = startReqID, .startAckID =
			startAckID, .endReqID = endReqID, .endAckID = endAckID, .canSend =
			canSend };
	link[n].link = syncLink;
	link[n].txCallback = txCallback;
	link[n].rxCallback = rxCallback;
	link[n].heap = heap;
	if(isQueue)
		link[n].txQueue = StaticQueue.new(heap,NULL);
	else
		link[n].txMap = StaticHashMap.new(heap);
	link[n].rxMap = StaticHashMap.new(heap);
	link[n].txCanQueue = StaticCANQueue.new();
	link[n].rxCanQueue = StaticCANQueue.new();
	link[n].isQueue = isQueue;
	return &link[n];
}

/**
 * This adds message to be transmitted without dynamic memory allocation
 * @param link	: Pointer to instance of CAN link
 * @param id	: ID of bytes to be transmitted
 * @param bytes	: Bytes to be transmitted
 * @param size	: Size in bytes
 */
static void addTxMsgPtr(CANLink *link, uint32_t id, uint8_t *bytes,
		uint16_t size) {
	SyncLayerCANData *syncData = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	syncData->id = id;
	syncData->bytes = bytes;
	syncData->isBytesDynamicallyAllocated = 0;
	syncData->size = size;
	syncData->numTry = 3;
	syncData->track = SYNC_LAYER_CAN_START_REQ;
	syncData->waitTill = 0xFFFFFFFF;

	if(link->isQueue)
		StaticQueue.enqueue(link->txQueue,syncData);
	else
		StaticHashMap.insert(link->txMap, id, syncData);
}

/**
 * This adds message to be transmitted with a dynamic memory allocation
 * @param link	: Pointer to instance of CAN link
 * @param id	: ID of bytes to be transmitted
 * @param bytes	: Bytes to be transmitted
 * @param size	: Size in bytes
 */
static void addTxMsg(CANLink *link, uint32_t id, uint8_t *bytes, uint16_t size) {
	SyncLayerCANData *syncData = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	uint8_t *allocatedBytes = StaticBuddyHeap.malloc(link->heap, size);
	for (int i = 0; i < size; i++)
		allocatedBytes[i] = bytes[i];

	syncData->id = id;
	syncData->bytes = allocatedBytes;
	syncData->isBytesDynamicallyAllocated = 1;
	syncData->size = size;
	syncData->numTry = 3;
	syncData->track = SYNC_LAYER_CAN_START_REQ;
	syncData->waitTill = 0xFFFFFFFF;
	if(link->isQueue)
		StaticQueue.enqueue(link->txQueue,syncData);
	else
		StaticHashMap.insert(link->txMap, id, syncData);
}

/**
 * This adds message to be received without dynamic memory allocation
 * @param link	: Pointer to instance if CAN link
 * @param id	: ID of bytes to be received
 * @param bytes	: Bytes to be received
 * @param size	: Size in bytes
 */
static void addRxMsgPtr(CANLink *link, uint32_t id, uint8_t *bytes,
		uint16_t size) {
	SyncLayerCANData *syncData = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	syncData->id = id;
	syncData->size = size;
	syncData->bytes = bytes;
	syncData->isBytesDynamicallyAllocated = 0;
	StaticHashMap.insert(link->rxMap, id, syncData);
}

static void txThread(CANLink *link) {
	if(link->isQueue){
		//Send thread
		SyncLayerCANData *syncData = StaticQueue.peek(link->txQueue);
		StaticSyncLayerCan.txSendThread(&link->link, syncData);

		int status = 0;
		if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS)
			status = link->txCallback(syncData->id, syncData->bytes,
					syncData->size, 1);
		else if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_FAILED)
			status = link->txCallback(syncData->id, syncData->bytes,
					syncData->size, 0);
		if (status) {
			if (syncData->isBytesDynamicallyAllocated)
				StaticBuddyHeap.free(link->heap, syncData->bytes);
			StaticBuddyHeap.free(link->heap, syncData);
			StaticQueue.dequeue(link->txQueue);
		}

		// Receive thread
		CANData canData = StaticCANQueue.dequeue(&link->txCanQueue);
		if (canData.ID != CAN_DATA_NULL.ID) {
			if (canData.ID == link->link.startAckID
					|| canData.ID == link->link.endAckID) {
				SyncLayerCANData *syncData = StaticQueue.peek(link->txQueue);
				StaticSyncLayerCan.txReceiveThread(&link->link, syncData,
						canData.ID, canData.byte, canData.len);
			}
		}
	}else{
		//Send thread
		int keyLen;
		int key[link->txMap->size];
		StaticHashMap.getKeys(link->txMap, key, &keyLen);
		for (int i = 0; i < keyLen; i++) {
			SyncLayerCANData *syncData = StaticHashMap.get(link->txMap, key[i]);


			StaticSyncLayerCan.txSendThread(&link->link, syncData);

			int status = 0;
			if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 1);
			else if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_FAILED)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 0);
			if (status) {
				if (syncData->isBytesDynamicallyAllocated)
					StaticBuddyHeap.free(link->heap, syncData->bytes);
				StaticBuddyHeap.free(link->heap, syncData);
				StaticHashMap.delete(link->txMap, key[i]);
			}
		}

		// Receive thread
		CANData canData = StaticCANQueue.dequeue(&link->txCanQueue);
		if (canData.ID != CAN_DATA_NULL.ID) {
			uint32_t dataID = canData.ID;
			if (canData.ID == link->link.startAckID
					|| canData.ID == link->link.endAckID) {
				dataID = *(uint32_t*) canData.byte;
				SyncLayerCANData *syncData = StaticHashMap.get(link->txMap, dataID);
				StaticSyncLayerCan.txReceiveThread(&link->link, syncData,
						canData.ID, canData.byte, canData.len);
			}
		}
	}
}

static void rxThread(CANLink *link) {
	//Send thread
	int keyLen;
	int key[link->rxMap->size];
	StaticHashMap.getKeys(link->rxMap, key, &keyLen);
	for (int i = 0; i < keyLen; i++) {
		SyncLayerCANData *syncData = StaticHashMap.get(link->rxMap, key[i]);
		StaticSyncLayerCan.rxSendThread(&link->link, syncData);

		int status = 0;
		if (syncData->track == SYNC_LAYER_CAN_RECEIVE_SUCCESS)
			status = link->rxCallback(syncData->id, syncData->bytes,
					syncData->size, 1);
		else if (syncData->track == SYNC_LAYER_CAN_RECEIVE_FAILED)
			status = link->rxCallback(syncData->id, syncData->bytes,
					syncData->size, 0);

		if (status) {
			if (syncData->isBytesDynamicallyAllocated)
				StaticBuddyHeap.free(link->heap, syncData->bytes);
			StaticBuddyHeap.free(link->heap, syncData);
			StaticHashMap.delete(link->rxMap, key[i]);
		}
	}

	// Receive thread
	CANData canData = StaticCANQueue.dequeue(&link->rxCanQueue);
	if (canData.ID != CAN_DATA_NULL.ID) {
		uint32_t dataID = canData.ID;
		SyncLayerCANData *syncData;
		if (canData.ID == link->link.startReqID) {
			dataID = *(uint32_t*) canData.byte;
			uint16_t size = *(uint16_t*) (&canData.byte[4]);
			if (!StaticHashMap.isKeyExist(link->rxMap, dataID)) {
				syncData = StaticBuddyHeap.malloc(link->heap,
						sizeof(SyncLayerCANData));
				syncData->isBytesDynamicallyAllocated = 1;
			}
			syncData->id = dataID;
			syncData->size = size;
			syncData->bytes = StaticBuddyHeap.malloc(link->heap, size);
			syncData->track = SYNC_LAYER_CAN_START_REQ;
			syncData->waitTill = 0xFFFFFFFF;
			StaticHashMap.insert(link->rxMap, dataID, syncData);
		} else {
			if (canData.ID == link->link.endReqID)
				dataID = *(uint32_t*) canData.byte;
			syncData = StaticHashMap.get(link->rxMap, dataID);
		}
		if (syncData != NULL)
			StaticSyncLayerCan.rxReceiveThread(&link->link, syncData,
					canData.ID, canData.byte, canData.len);
	}

}

/**
 * This should be called continuously in while loop
 * @param link	: Pointer to instance of CAN link
 */
static void thread(CANLink *link) {
	txThread(link);
	rxThread(link);
}

/**
 * This should be called when CAN message is received
 * @param link	: Pointer to instance if CAN link
 * @param id	: ID of bytes received
 * @param bytes	: Bytes received
 */
static void canReceive(CANLink *link, uint32_t id, uint8_t *bytes, uint16_t len) {
	if (id == link->link.startAckID || id == link->link.endAckID) {
		StaticCANQueue.enqueue(&link->txCanQueue, id, bytes, len);
	} else if (id == link->link.startReqID || id == link->link.endReqID) {
		StaticCANQueue.enqueue(&link->rxCanQueue, id, bytes, len);
	} else {
		if (StaticHashMap.isKeyExist(link->rxMap, id))
			StaticCANQueue.enqueue(&link->rxCanQueue, id, bytes, len);
	}
}

struct CANLinkControl StaticCANLink = {
		.addRxMsgPtr = addRxMsgPtr,
		.addTxMsg = addTxMsg,
		.addTxMsgPtr = addTxMsgPtr,
		.canReceive = canReceive,
		.new = new,
		.thread = thread,
};
