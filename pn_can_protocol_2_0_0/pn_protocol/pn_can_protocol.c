/*
 * pn_can_protocol.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "pn_can_protocol.h"
#include "stdarg.h"
#include "stdio.h"
#include "main.h"
#include "string.h"

//#define CONSOLE_ENABLE

#ifdef CONSOLE_ENABLE
#define TAG "PN_SYNC"
typedef enum {
	CONSOLE_ERROR, CONSOLE_INFO, CONSOLE_WARNING
} ConsoleStatus;
static void console(ConsoleStatus status, const char *func_name,
		const char *msg, ...) {
//	if(status == CONSOLE_INFO)
//		return;
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
static int allocatedMemory = 0;

/**
 * It allocates the memory and return pointer to it
 * @param heap          : Pointer to static heap
 *                      : NULL for dynamic heap
 * @param sizeInByte    : Size in bytes
 * @return              : Pointer to allocated memory
 *                      : NULL if there exist no memory for allocation
 */
static void* allocateMemory(BuddyHeap *heap, int sizeInByte) {
	if (sizeInByte <= 0)
		return NULL;
	void *ptr;
	ptr = heap != NULL ?
			StaticBuddyHeap.malloc(heap, sizeInByte) : malloc(sizeInByte);
	if (ptr != NULL)
		allocatedMemory += sizeInByte;
	return ptr;
}

/**
 * It free the allocated memory
 * @param heap          : Pointer to static heap
 *                      : NULL for dynamic heap
 * @param pointer       : Pointer to allocated Memory
 * @param sizeInByte    : Size to be freed
 * @return              : 1 for success (OR) 0 for failed
 */
static int freeMemory(BuddyHeap *heap, void *pointer, int sizeInByte) {
	if (pointer == NULL || sizeInByte <= 0)
		return 0;
	heap != NULL ? StaticBuddyHeap.free(heap, pointer) : free(pointer);
	allocatedMemory -= sizeInByte;
	return 1;
}

static uint32_t getMillis() {
	return HAL_GetTick();
}

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
static CANLink* new(uint32_t startReqID, uint32_t startAckID, uint32_t endReqID,
		uint32_t endAckID,
		int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
		int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
				int status,const char*log),
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
	if (isQueue) {
		link[n].txQueue = StaticQueue.new(heap, NULL);
		if (link[n].txQueue == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Link [0x%x, 0x%x, 0x%x, 0x%x] creation : FAILED (Heap is full) \n",
					startReqID, startAckID, endReqID, endAckID);
#endif
			return NULL;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Transmit Queue for link [0x%x, 0x%x, 0x%x, 0x%x] creation : SUCCESS \n",
					startReqID, startAckID, endReqID, endAckID);
#endif
	} else {
		link[n].txMap = StaticHashMap.new(heap);
		if (link[n].txMap == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Transmit Queue for link [0x%x, 0x%x, 0x%x, 0x%x] creation : FAILED (Heap is full) \n",
					startReqID, startAckID, endReqID, endAckID);
#endif
			return NULL;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Transmit Map for link [0x%x, 0x%x, 0x%x, 0x%x] creation : SUCCESS \n",
					startReqID, startAckID, endReqID, endAckID);
#endif
	}
	link[n].rxMap = StaticHashMap.new(heap);
	if (link[n].rxMap == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Receive map for link [0x%x, 0x%x, 0x%x, 0x%x] creation : FAILED (Heap is full) \n",
				startReqID, startAckID, endReqID, endAckID);
#endif
		if (isQueue)
			freeMemory(heap, link[n].txQueue, sizeof(Queue));
		else
			freeMemory(heap, link[n].txMap, sizeof(HashMap));
		return NULL;
	}
#ifdef CONSOLE_ENABLE
	else
		console(CONSOLE_INFO, __func__,
				"Receive Map for link [0x%x, 0x%x, 0x%x, 0x%x] creation : SUCCESS\n",
				startReqID, startAckID, endReqID, endAckID);
#endif
	link[n].txCanQueue = StaticCANQueue.new();
	link[n].rxCanQueue = StaticCANQueue.new();
	link[n].isQueue = isQueue;
#ifdef CONSOLE_ENABLE
	console(CONSOLE_INFO, __func__,
			"Link [0x%x, 0x%x, 0x%x, 0x%x] creation : SUCCESS\n", startReqID,
			startAckID, endReqID, endAckID);
#endif
	return &link[n];
}

static SyncLayerCANData* getDataFromQueue(Queue *queue, uint32_t ID) {
	int que_size = queue->size;
	struct QueueData *queData = queue->front;
	SyncLayerCANData *syncData;
	for (int i = 0; i < que_size; i++) {
		syncData = (SyncLayerCANData*) queData->value;
		if (syncData->id == ID)
			return syncData;
		queData = queData->next;
		if (queData == NULL)
			break;
	}
	return NULL;
}
/**
 * This adds message to be transmitted without dynamic memory allocation
 * @param link	: Pointer to instance of CAN link
 * @param id	: ID of bytes to be transmitted
 * @param bytes	: Bytes to be transmitted
 * @param size	: Size in bytes
 * @param log	: Log of message
 */
static void addTxMsgPtr(CANLink *link, uint32_t id, uint8_t *bytes,
		uint16_t size, const char *log) {
	if (bytes == NULL)
		return;
	SyncLayerCANData *syncData;
	int isSyncDataAllocated = 0;
	if (!link->isQueue) {
		syncData = StaticHashMap.get(link->txMap, id);
		if (syncData == NULL) {
			syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
			isSyncDataAllocated = 1;
		}
	} else {
		syncData = getDataFromQueue(link->txQueue, id);
		if (syncData == NULL) {
			syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
			isSyncDataAllocated = 1;
		}
	}
	if (syncData == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Sync data for data 0x%x creation : FAILED (Heap is full) \n",
				id);
#endif
		return;
	}
#ifdef CONSOLE_ENABLE
	else
		console(CONSOLE_INFO, __func__,
				"Sync data for data 0x%x creation : SUCCESS \n", id);
#endif
	if (!isSyncDataAllocated)
		return;
	syncData->id = id;
	syncData->bytes = bytes;
	syncData->isBytesDynamicallyAllocated = 0;
	syncData->size = size;
	syncData->numTry = PN_CAN_PROTOCOL_NUM_OF_TRY;
	syncData->track = SYNC_LAYER_CAN_START_REQ;
	syncData->waitTill = getMillis() + TRANSMIT_TIMEOUT; //b0xFFFFFFFF;
	syncData->log[0] = '\0';
	strcpy(syncData->log, log);

	if (link->isQueue) {
		Queue *queue = StaticQueue.enqueue(link->txQueue, syncData);
		if (queue == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Data 0x%x insertion in transmit queue : FAILED (Queue is full) \n",
					id);
#endif
			freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
			return;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Data 0x%x insertion in transmit queue : SUCCESS \n", id);
#endif
	} else {
		HashMap *map = StaticHashMap.insert(link->txMap, id, syncData);
		if (map == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Data 0x%x insertion in transmit map : FAILED (Map is full) \n",
					id);
#endif
			freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
			return;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Data 0x%x insertion in transmit map : SUCCESS \n", id);
#endif
	}
}

/**
 * This adds message to be transmitted with a dynamic memory allocation
 * @param link	: Pointer to instance of CAN link
 * @param id	: ID of bytes to be transmitted
 * @param bytes	: Bytes to be transmitted
 * @param size	: Size in bytes
 * @param log	: Log of message
 */
static void addTxMsg(CANLink *link, uint32_t id, uint8_t *bytes, uint16_t size,
		const char *log) {
	if (bytes == NULL)
		return;

	SyncLayerCANData *syncData;
	int isSyncDataAllocated = 0;
	if (!link->isQueue) {
		syncData = StaticHashMap.get(link->txMap, id);
		if (syncData == NULL) {
			syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
			isSyncDataAllocated = 1;
		}
	} else {
		syncData = getDataFromQueue(link->txQueue, id);
		if (syncData == NULL) {
			syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
			isSyncDataAllocated = 1;
		}
	}
	if (syncData == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Sync data for data 0x%x creation : FAILED (Heap is full) \n",
				id);
#endif
		return;
	}
#ifdef CONSOLE_ENABLE
	else
		console(CONSOLE_INFO, __func__,
				"Sync data for data 0x%x creation : SUCCESS\n", id);
#endif

	uint8_t *allocatedBytes;
	if (!isSyncDataAllocated)
		allocatedBytes = allocateMemory(link->heap, size);
	else
		allocatedBytes = syncData->bytes;

	if (allocatedBytes == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Bytes for data 0x%x allocation : FAILED (Heap is full) \n",
				id);
#endif
		freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
		return;
	}
#ifdef CONSOLE_ENABLE
	else {
		if (!isSyncDataAllocated)
			console(CONSOLE_INFO, __func__,
					"Bytes for data 0x%x allocation : SUCCESS\n", id);
	}
#endif
	for (int i = 0; i < size; i++)
		allocatedBytes[i] = bytes[i];

	if (isSyncDataAllocated)
		return;

	syncData->id = id;
	syncData->bytes = allocatedBytes;
	syncData->isBytesDynamicallyAllocated = 1;
	syncData->size = size;
	syncData->numTry = PN_CAN_PROTOCOL_NUM_OF_TRY;
	syncData->track = SYNC_LAYER_CAN_START_REQ;
	syncData->waitTill = getMillis() + TRANSMIT_TIMEOUT;
	syncData->log[0] = '\0';
	strcpy(syncData->log, log);

	if (link->isQueue) {
		Queue *queue = StaticQueue.enqueue(link->txQueue, syncData);
		if (queue == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Data 0x%x insertion in transmit queue : FAILED (Queue is full) \n",
					id);
#endif
			freeMemory(link->heap, allocatedBytes, syncData->size);
			freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
			return;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Data 0x%x insertion in transmit queue : SUCCESS \n", id);
#endif
	} else {
		HashMap *map = StaticHashMap.insert(link->txMap, id, syncData);
		if (map == NULL) {
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__,
					"Data 0x%x insertion in transmit map : FAILED (Map is full) \n",
					id);
#endif
			freeMemory(link->heap, allocatedBytes, syncData->size);
			freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
			return;
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_INFO, __func__,
					"Data 0x%x insertion in transmit map : SUCCESS \n", id);
#endif
	}
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
	if (bytes == NULL)
		return;
	SyncLayerCANData *syncData;
	int isSyncDataAllocated = 0;
	syncData = StaticHashMap.get(link->rxMap, id);
	if (syncData == NULL) {
		syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
		isSyncDataAllocated = 1;
	}
	if (syncData == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Sync data for data 0x%x creation : FAILED (Heap is full) \n",
				id);
#endif
		return;
	}
#ifdef CONSOLE_ENABLE
	else
		console(CONSOLE_INFO, __func__,
				"Sync data for data 0x%x creation : SUCCESS \n", id);
#endif
	if (!isSyncDataAllocated)
		return;
	syncData->id = id;
	syncData->size = size;
	syncData->bytes = bytes;
	syncData->waitTill = getMillis() + RECEIVE_TIMEOUT;
	syncData->isBytesDynamicallyAllocated = 0;
	HashMap *map = StaticHashMap.insert(link->rxMap, id, syncData);
	if (map == NULL) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__,
				"Data 0x%x insertion in receive map : FAILED (Map is full) \n",
				id);
#endif
		freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
		return;
	}
#ifdef CONSOLE_ENABLE
	else
		console(CONSOLE_INFO, __func__,
				"Data 0x%x insertion in transmit map : SUCCESS \n)", id);
#endif
}

/**
 * This will pop the data in link
 * @param link		: Pointer to instance if CAN link (Link should use queue for transmit)
 * @return			: 1 for popped and
 * 					: 0 for fail to pop or link is using map to transmit
 */
static int pop(CANLink *link) {
	if (!link->isQueue)
		return 0;
	SyncLayerCANData *syncData = StaticQueue.dequeue(link->txQueue);
	if (syncData->isBytesDynamicallyAllocated)
		freeMemory(link->heap, syncData->bytes, syncData->size);
	freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
	return 1;
}

static void txThread(CANLink *link) {
	if (link->isQueue) {
		//Send thread
		SyncLayerCANData *syncData = StaticQueue.peek(link->txQueue);
		if (syncData != NULL) {
			if(syncData->track!=SYNC_LAYER_CAN_TRANSMIT_SUCCESS && syncData->track!=SYNC_LAYER_CAN_TRANSMIT_FAILED)
				StaticSyncLayerCan.txSendThread(&link->link, syncData);

			int status = 0;
			if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 1,syncData->log);
			else if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_FAILED)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 0,syncData->log);
			if (status) {
				if (syncData->isBytesDynamicallyAllocated)
					freeMemory(link->heap, syncData->bytes, syncData->size);
				freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
				StaticQueue.dequeue(link->txQueue);
			}
		}

		// Receive thread
		CANData canData = StaticCANQueue.dequeue(&link->txCanQueue);
		if (canData.ID != CAN_DATA_NULL.ID) {
			if (canData.ID == link->link.startAckID
					|| canData.ID == link->link.endAckID) {
				SyncLayerCANData *syncData = StaticQueue.peek(link->txQueue);
				if (syncData != NULL)
					if(syncData->track!=SYNC_LAYER_CAN_TRANSMIT_SUCCESS && syncData->track!=SYNC_LAYER_CAN_TRANSMIT_FAILED)
						StaticSyncLayerCan.txReceiveThread(&link->link, syncData,
								canData.ID, canData.byte, canData.len);
			}
		}
	} else {
		//Send thread
		int keyLen;
		int key[link->txMap->size];
		StaticHashMap.getKeys(link->txMap, key, &keyLen);
		for (int i = 0; i < keyLen; i++) {
			SyncLayerCANData *syncData = StaticHashMap.get(link->txMap, key[i]);
			if(syncData->track!=SYNC_LAYER_CAN_TRANSMIT_SUCCESS && syncData->track!=SYNC_LAYER_CAN_TRANSMIT_FAILED)
				StaticSyncLayerCan.txSendThread(&link->link, syncData);

			int status = 0;
			if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 1,syncData->log);
			else if (syncData->track == SYNC_LAYER_CAN_TRANSMIT_FAILED)
				status = link->txCallback(syncData->id, syncData->bytes,
						syncData->size, 0,syncData->log);
			if (status) {
				if (syncData->isBytesDynamicallyAllocated)
					freeMemory(link->heap, syncData->bytes, syncData->size);
				freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
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
				SyncLayerCANData *syncData = StaticHashMap.get(link->txMap,
						dataID);
				if (syncData != NULL)
					if(syncData->track!=SYNC_LAYER_CAN_TRANSMIT_SUCCESS && syncData->track!=SYNC_LAYER_CAN_TRANSMIT_FAILED)
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
		if(syncData->track!=SYNC_LAYER_CAN_RECEIVE_SUCCESS && syncData->track!=SYNC_LAYER_CAN_RECEIVE_FAILED)
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
				freeMemory(link->heap, syncData->bytes, syncData->size);
			freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
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
				syncData = allocateMemory(link->heap, sizeof(SyncLayerCANData));
				if (syncData == NULL) {
#ifdef CONSOLE_ENABLE
					console(CONSOLE_ERROR, __func__,
							"Sync data for data 0x%x creation : FAILED (heap is full)\n",
							dataID);
#endif
					return;
				}
#ifdef CONSOLE_ENABLE
				else
					console(CONSOLE_INFO, __func__,
							"Sync data for data 0x%x creation : SUCCESS\n",
							dataID);
#endif
				syncData->isBytesDynamicallyAllocated = 1;
				syncData->id = dataID;
				syncData->size = size;
				syncData->bytes = allocateMemory(link->heap, size);
				if (syncData->bytes == NULL) {
#ifdef CONSOLE_ENABLE
					console(CONSOLE_ERROR, __func__,
							"Bytes for data 0x%x allocation : FAILED (heap is full)\n",
							dataID);
#endif
					freeMemory(link->heap, syncData, sizeof(SyncLayerCANData));
					return;
				}
#ifdef CONSOLE_ENABLE
				else
					console(CONSOLE_INFO, __func__,
							"Bytes for data 0x%x allocation : SUCCESS\n",
							dataID);
#endif
				syncData->track = SYNC_LAYER_CAN_START_REQ;
				syncData->waitTill = getMillis() + RECEIVE_TIMEOUT;
				StaticHashMap.insert(link->rxMap, dataID, syncData);
			}
		} else {
			if (canData.ID == link->link.endReqID)
				dataID = *(uint32_t*) canData.byte;
			syncData = StaticHashMap.get(link->rxMap, dataID);
		}
		if (syncData != NULL)
			if(syncData->track!=SYNC_LAYER_CAN_RECEIVE_SUCCESS && syncData->track!=SYNC_LAYER_CAN_RECEIVE_FAILED)
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

struct CANLinkControl StaticCANLink = { .addRxMsgPtr = addRxMsgPtr, .addTxMsg =
		addTxMsg, .addTxMsgPtr = addTxMsgPtr, .pop = pop, .canReceive =
		canReceive, .new = new, .thread = thread, };
