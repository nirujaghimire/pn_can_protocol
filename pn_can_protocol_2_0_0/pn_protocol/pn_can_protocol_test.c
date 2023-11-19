/*
 * pn_can_protocol_test.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "main.h"
#include "pn_can_sync_layer.h"
#include "hash_map.h"
#include "buddy_heap.h"
#include "CAN_queue.h"
#include "stdlib.h"

#define TEST_ENABLE
#define TX_ENABLE

#ifdef TEST_ENABLE

typedef struct {
	SyncLayerCANLink link;
	int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size, int status);
	int (*rxCallback)(uint32_t id, uint8_t *bytes, uint16_t size, int status);
	BuddyHeap *heap;
	HashMap *txMap;
	HashMap *rxMap;
	CANQueue txCanQueue;
	CANQueue rxCanQueue;
} CanLink;

static CanLink link[10];
static int linkCount = 0;

static CanLink* newLink(uint32_t startReqID, uint32_t startAckID,
		uint32_t endReqID, uint32_t endAckID,
		int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
		int (*txCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
				int status),
		int (*rxCallback)(uint32_t id, uint8_t *bytes, uint16_t size,
				int status), BuddyHeap *heap) {

	int n = linkCount++;

	SyncLayerCANLink syncLink = { .startReqID = startReqID, .startAckID =
			startAckID, .endReqID = endReqID, .endAckID = endAckID, .canSend =
			canSend };
	link[n].link = syncLink;
	link[n].txCallback = txCallback;
	link[n].rxCallback = rxCallback;
	link[n].heap = heap;
	link[n].txMap = StaticHashMap.new(heap);
	link[n].rxMap = StaticHashMap.new(heap);
	link[n].txCanQueue = StaticCANQueue.new();
	link[n].rxCanQueue = StaticCANQueue.new();
	return &link[n];
}

static void addTxMsgPtr(CanLink *link, uint32_t id, uint8_t *bytes,
		uint16_t size) {
	SyncLayerCANData *syndData = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	syndData->id = id;
	syndData->bytes = bytes;
	syndData->isBytesDynamicallyAllocated = 0;
	syndData->size = size;
	syndData->numTry = 3;
	syndData->track = SYNC_LAYER_CAN_START_REQ;
	syndData->waitTill = 0xFFFFFFFF;
	StaticHashMap.insert(link->txMap, id, syndData);
}

static void addTxMsg(CanLink *link, uint32_t id, uint8_t *bytes, uint16_t size) {
	SyncLayerCANData *syndData = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	uint8_t* allocatedBytes = StaticBuddyHeap.malloc(link->heap, size);
	for (int i = 0; i < size; i++)
		allocatedBytes[i] = bytes[i];

	syndData->id = id;
	syndData->bytes = allocatedBytes;
	syndData->isBytesDynamicallyAllocated = 1;
	syndData->size = size;
	syndData->numTry = 3;
	syndData->track = SYNC_LAYER_CAN_START_REQ;
	syndData->waitTill = 0xFFFFFFFF;
	StaticHashMap.insert(link->txMap, id, syndData);
}

static void addRxMsgPtr(CanLink *link, uint32_t id, uint8_t *bytes,
		uint16_t size) {;
	SyncLayerCANData* syncData = StaticBuddyHeap.malloc(link->heap, sizeof(SyncLayerCANData));
	syncData->id = id;
	syncData->size = size;
	syncData->bytes = bytes;
	syncData->isBytesDynamicallyAllocated = 0;
	StaticHashMap.insert(link->rxMap, id, syncData);
}

static void txThread(CanLink *link) {
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
			StaticHashMap.delete(link->txMap,key[i]);
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

static void rxThread(CanLink *link) {
	//Send thread
	int keyLen;
	int key[link->rxMap->size];
	StaticHashMap.getKeys(link->rxMap, key, &keyLen);
	for (int i = 0; i < keyLen; i++) {
		SyncLayerCANData *syncData = StaticHashMap.get(link->rxMap, key[i]);
		StaticSyncLayerCan.rxSendThread(&link->link, syncData);

		int status = 0;
		if (syncData->track == SYNC_LAYER_CAN_RECEIVE_SUCCESS)
			status = link->rxCallback(syncData->id, syncData->bytes,syncData->size, 1);
		else if (syncData->track == SYNC_LAYER_CAN_RECEIVE_FAILED)
			status = link->rxCallback(syncData->id, syncData->bytes,syncData->size, 0);

		if (status) {
			if (syncData->isBytesDynamicallyAllocated)
				StaticBuddyHeap.free(link->heap, syncData->bytes);
			StaticBuddyHeap.free(link->heap, syncData);
			StaticHashMap.delete(link->rxMap,key[i]);
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
			if(!StaticHashMap.isKeyExist(link->rxMap,dataID)){
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

static void thread(CanLink *link) {
	txThread(link);
	rxThread(link);
}

static void canReceive(CanLink *link, uint32_t id, uint8_t *bytes, uint16_t len) {
	if (id == link->link.startAckID || id == link->link.endAckID) {
		StaticCANQueue.enqueue(&link->txCanQueue, id, bytes, len);
	} else if (id == link->link.startReqID || id == link->link.endReqID) {
		StaticCANQueue.enqueue(&link->rxCanQueue, id, bytes, len);
	} else {
		if (StaticHashMap.isKeyExist(link->rxMap, id))
			StaticCANQueue.enqueue(&link->rxCanQueue, id, bytes, len);
	}
}

////////////////////////////////////////
static CanLink *canLink;

extern CAN_HandleTypeDef hcan;
static void canInit() {
	CAN_FilterTypeDef can_filter;

	can_filter.FilterActivation = CAN_FILTER_ENABLE;
	can_filter.FilterBank = 0;
	can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	can_filter.FilterIdHigh = 0x0;
	can_filter.FilterIdLow = 0x0;
	can_filter.FilterMaskIdHigh = 0x0;
	can_filter.FilterMaskIdLow = 0x0;
	can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
	can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
	can_filter.SlaveStartFilterBank = 0;

	HAL_CAN_ConfigFilter(&hcan, &can_filter);
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING)
			!= HAL_OK)
		printf("CAN RX Interrupt Activate : Failed\n");
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
		printf("CAN TX Interrupt Activate : Failed\n");
	if (HAL_CAN_Start(&hcan) != HAL_OK)
		printf("CAN Start : Failed\n");
}

static CAN_RxHeaderTypeDef rx_header;
static uint8_t bytes[8];
void canInterrupt() {
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, bytes);
//	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
//	for (int i = 0; i < rx_header.DLC; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");
	canReceive(canLink, rx_header.ExtId, bytes, rx_header.DLC);
}

static CAN_TxHeaderTypeDef tx_header;
static uint32_t tx_mailbox;
static int canSend(uint32_t id, uint8_t *bytes, uint8_t len) {
	tx_header.DLC = len;
	tx_header.ExtId = id;
	tx_header.IDE = CAN_ID_EXT;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.TransmitGlobalTime = DISABLE;

//	printf("canSend-> 0x%02x : ", (unsigned int) id);
//	for (int i = 0; i < len; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");
	return HAL_CAN_AddTxMessage(&hcan, &tx_header, bytes, &tx_mailbox) == HAL_OK;
}

int txCallback(uint32_t id, uint8_t *bytes, uint16_t size, int status) {
	printf("TX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);
	if (!status) {
		printf("(failed)\n");
		return 0;
	}
	printf("\n");
	return 1;
}

int rxCallback(uint32_t id, uint8_t *bytes, uint16_t size, int status) {
	printf("RX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);
	if (!status) {
		printf("(failed)\n");
		return 0;
	}
	printf("\n");
	return 1;
}

uint8_t dataBytes[16];
uint8_t buffer[1024 + mapSize(1024, 8)];
#ifdef TX_ENABLE
void run() {
	canInit();
	for (int i = 0; i < sizeof(dataBytes); i++)
		dataBytes[i] = i;

	printf("----------------------TX INITIATING-----------------------\n");
	HAL_Delay(1000);

	BuddyHeap heap = StaticBuddyHeap.new(buffer,sizeof(buffer),8);
	canLink = newLink(0x1, 0x2, 0x3, 0x4, canSend, txCallback, rxCallback, &heap);
	addTxMsgPtr(canLink, 0xA, dataBytes, sizeof(dataBytes));

	uint32_t prevMillis = HAL_GetTick();
	while (1) {
		if((HAL_GetTick()-prevMillis)>10){
			addTxMsg(canLink, 0xA, dataBytes, sizeof(dataBytes));
			prevMillis = HAL_GetTick();
		}
		thread(canLink);
//		HAL_Delay(100);
	}
}
#else
void run() {
	canInit();
	for (int i = 0; i < sizeof(dataBytes); i++)
		dataBytes[i] = i;

	printf("----------------------RX INITIATING-----------------------\n");
	HAL_Delay(1000);

	BuddyHeap heap = StaticBuddyHeap.new(buffer, sizeof(buffer), 8);
	canLink = newLink(0x1, 0x2, 0x3, 0x4, canSend, txCallback, rxCallback,
			&heap);

	uint32_t prevMillis = HAL_GetTick();
	while (1) {
//		if((HAL_GetTick()-prevMillis)>10000){
//			addTxMsg(canLink, 0xA1, dataBytes, sizeof(dataBytes));
//			prevMillis = HAL_GetTick();
//		}

		thread(canLink);
//		HAL_Delay(100);
	}
}
#endif

#endif
