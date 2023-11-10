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

#define TEST_ENABLE

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

static void addTxMsg(CanLink *link, uint32_t id, uint8_t *bytes, uint16_t size) {
	SyncLayerCANData *data = StaticBuddyHeap.malloc(link->heap,
			sizeof(SyncLayerCANData));
	data->id = id;
	data->bytes = bytes;
	data->size = size;
	data->numTry = 3;
	data->track = SYNC_LAYER_CAN_START_REQ;
	data->waitTill = 0xFFFFFFFF;
	StaticHashMap.insert(link->txMap, id, data);
}

static void txThread(CanLink *link) {
	//Send thread
	int keyLen;
	int key[link->txMap->size];
	StaticHashMap.getKeys(link->txMap, key, &keyLen);
	for (int i = 0; i < keyLen; i++) {
		SyncLayerCANData *data = StaticHashMap.get(link->txMap, key[i]);
		StaticSyncLayerCan.txSendThread(&link->link, data);

		if (data->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS) {
			link->txCallback(data->id, data->bytes, data->size, 1);
		} else if (data->track == SYNC_LAYER_CAN_TRANSMIT_FAILED) {
			link->txCallback(data->id, data->bytes, data->size, 0);
		}
	}
	// Receive thread
	CANData canData = StaticCANQueue.dequeue(&link->txCanQueue);
	uint32_t dataID = canData.ID;
	if (canData.ID == link->link.startAckID
			|| canData.ID == link->link.endAckID)
		dataID = *(uint32_t*) canData.byte;
	SyncLayerCANData *syncData = StaticHashMap.get(link->txMap, dataID);
	StaticSyncLayerCan.txReceiveThread(&link->link, syncData, canData.ID,
			canData.byte, canData.len);
}

static void rxThread(CanLink *link) {
	//Send thread
	int keyLen;
	int key[link->txMap->size];
	StaticHashMap.getKeys(link->txMap, key, &keyLen);
	for (int i = 0; i < keyLen; i++) {
		SyncLayerCANData *data = StaticHashMap.get(link->rxMap, key[i]);
		StaticSyncLayerCan.txSendThread(&link->link, data);

		if (data->track == SYNC_LAYER_CAN_RECEIVE_SUCCESS) {
			link->rxCallback(data->id, data->bytes, data->size, 1);
		} else if (data->track == SYNC_LAYER_CAN_RECEIVE_FAILED) {
			link->rxCallback(data->id, data->bytes, data->size, 0);
		}
	}

	// Receive thread
	CANData canData = StaticCANQueue.dequeue(&link->txCanQueue);
	uint32_t dataID = canData.ID;
	SyncLayerCANData *syncData;
	if (canData.ID == link->link.startAckID) {
		dataID = *(uint32_t*) canData.byte;

		syncData = StaticBuddyHeap.malloc(link->heap, sizeof(SyncLayerCANData));
		syncData->id = dataID;
		syncData->track = SYNC_LAYER_CAN_START_REQ;
		syncData->waitTill = 0xFFFFFFFF;
		StaticHashMap.insert(link->rxMap, dataID, syncData);
	} else {
		if (canData.ID == link->link.endAckID)
			dataID = *(uint32_t*) canData.byte;
		syncData = StaticHashMap.get(link->rxMap, dataID);
	}
	StaticSyncLayerCan.rxReceiveThread(&link->link, syncData, canData.ID,
			canData.byte, canData.len);
}


static void thread(CanLink *link){
	txThread(link);
	rxThread(link);
}

static void canReceive(CanLink *link, uint32_t id, uint8_t *bytes, uint16_t len) {
	if (id == link->link.startAckID || id == link->link.endAckID) {
		StaticCANQueue.enqueue(&link->txCanQueue, id, bytes, len);
	} else {
		if (StaticHashMap.isKeyExist(link->txMap, id))
			StaticCANQueue.enqueue(&link->txCanQueue, id, bytes, len);
	}

}

////////////////////////////////////////
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

void txCallback(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("TX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);
	if (!status) {
		printf("(failed)\n");
		return;
	}
	printf("\n");
}

uint8_t tx_bytes[70];
void runTx() {
	canInit();

	for (int i = 0; i < sizeof(tx_bytes); i++)
		tx_bytes[i] = i;

	printf("----------------------INITIATING-----------------------\n");
	HAL_Delay(1000);

	while (1) {

	}
}

#endif
