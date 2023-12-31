/*
 * pn_can_protocol_test.c
 *
 *  Created on: Jul 10, 2023
 *      Author: NIRUJA
 */

#include "pn_can_protocol.h"
#include "main.h"
#include "malloc.h"
#include "hash_map.h"

static SyncLayerCanLink link1 = { 1, 2, 3, 4, 5, 6, 7 };
static SyncLayerCanLink link2 = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

/********************CONSOLE***************************/
static void console(const char *title, const char *msg) {
	printf("%s:: %s\n", title, msg);
}

/************************CAN****************************/
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
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
		console("CAN RX Interrupt Activate", "Failed");
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
		console("CAN TX Interrupt Activate", "Failed");
	if (HAL_CAN_Start(&hcan) != HAL_OK)
		console("CAN Start", "Failed");
}

static CAN_RxHeaderTypeDef rx_header;
static uint8_t bytes[8];
static void canRxInterrupt() {
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, bytes);
	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
	for (int i = 0; i < rx_header.DLC; ++i)
		printf("%d ", bytes[i]);
	printf("\n");
	StaticCanProtocol.recCAN(&link1, rx_header.ExtId, bytes,rx_header.DLC);
	StaticCanProtocol.recCAN(&link2, rx_header.ExtId, bytes,rx_header.DLC);

}

static CAN_TxHeaderTypeDef tx_header;
static uint32_t tx_mailbox;
static uint8_t canSend(uint32_t id, uint8_t *bytes, uint8_t len) {

	tx_header.DLC = len;
	tx_header.ExtId = id;
	tx_header.IDE = CAN_ID_EXT;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.TransmitGlobalTime = DISABLE;

	printf("canSend-> 0x%02x : ", (unsigned int) id);
	for (int i = 0; i < len; ++i)
		printf("%d ", bytes[i]);
	printf("\n");

	return HAL_CAN_AddTxMessage(&hcan, &tx_header, bytes, &tx_mailbox) == HAL_OK;
}

/**********************MAIN THREAD****************************/
static uint8_t txCallback1(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("Tx Data1 : ");
	if (!status) {
		printf("failed\n");
		return 1;
	}
	printf("0x%0x -> ", (int) id);
	for (int i = 0; i < size; i++)
		printf("%d ", bytes[i]);
	printf("\n");
	return 1;
}

static uint8_t rxCallback1(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("Rx Data1 : ");
	if (!status) {
		printf("failed\n");
		return 1;
	}
	printf("0x%0x -> ", (int) id);
	for (int i = 0; i < size; i++)
		printf("%d ", bytes[i]);
	printf("\n");

//	StaticCanProtocol.addTxMessage(&link1,id+10,bytes,size);

	return 1;
}

static uint8_t txCallback2(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("Tx Data2 : ");
	if (!status) {
		printf("failed\n");
		return 1;
	}
	printf("0x%0x -> ", (int) id);
	for (int i = 0; i < size; i++)
		printf("%d ", bytes[i]);
	printf("\n");
	return 1;
}

static uint8_t rxCallback2(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("Rx Data2 : ");
	if (!status) {
		printf("failed\n");
		return 1;
	}
	printf("0x%0x -> ", (int) id);
	for (int i = 0; i < size; i++)
		printf("%d ", bytes[i]);
	printf("\n");

	return 1;
}

uint8_t buffer[1024 + mapSize(1024, 4)];
BuddyHeap heap;
static uint8_t tx_bytes1[16] = { 1, 2, 3, 4, [7]=10 };
static uint8_t tx_bytes2[16] = { 1, 5, 3, 7 };
static uint8_t tx_bytes3[16] = { 2, 4, 6, 8 };
static uint8_t tx_bytes4[16] = { 2, 4, 6, 8 };

static void runRx() {
	extern void printQueue();

	heap = StaticBuddyHeap.new(buffer, sizeof(buffer), 4);
	StaticCanProtocol.addLink(&heap, &link1, canSend, txCallback1, rxCallback1, 0);
	StaticCanProtocol.addLink(&heap, &link2, canSend, txCallback2, rxCallback2, 0);
	canInit();

	console("\n\nSOURCE INIT", "SUCCESS");

	printf("MEMORY_SIZE : %d\n", heap.maxSize);
	printf("MIN_MEMORY_SIZE : %d\n", heap.minSize);
	printf("MAP_SIZE : %d\n", heap.mapSize);

	printf("Heap : %p\n", &heap);
	printf("Memory : %p - %p\n", heap.memory, heap.memory + heap.maxSize);
	printf("Map : %p - %p\n\n", heap.map, heap.map + heap.mapSize);
	HAL_Delay(1000);

	uint32_t tick = HAL_GetTick();
	uint32_t prev_milis = HAL_GetTick();
	while (1) {
//		uint32_t current_milis = HAL_GetTick();
//		if ((current_milis - prev_milis) > 1000) {
//			printf("###################################################################\n");
//			printf("Memory : %p - %p\n", heap.memory, heap.memory + heap.maxSize);
////			StaticBuddyHeap.printMap(heap);
//			prev_milis = HAL_GetTick();
//			printQueue();
//		}
//		uint32_t tock = HAL_GetTick();
//		if ((tock - tick) >= 100) {
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xA1, tx_bytes1, sizeof(tx_bytes1));
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xB1, tx_bytes2, sizeof(tx_bytes2));
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xC1, tx_bytes3, sizeof(tx_bytes3));
//		StaticCanProtocol.addTxMessagePtr(&link2, 0xD1, tx_bytes4, sizeof(tx_bytes4));
//		StaticCanProtocol.addTxMessagePtr(&link2, 0xE1, tx_bytes3, sizeof(tx_bytes3));
//		tick = tock;
//		}
		StaticCanProtocol.thread(&link1);
//		StaticCanProtocol.thread(&link2);
		HAL_Delay(1000);
	}
}

static void runTx() {
	extern void printQueue();

	heap = StaticBuddyHeap.new(buffer, sizeof(buffer), 4);
	StaticCanProtocol.addLink(&heap, &link1, canSend, txCallback1, rxCallback1, 0);
	StaticCanProtocol.addLink(&heap, &link2, canSend, txCallback2, rxCallback2, 0);
	canInit();

	console("\n\nSOURCE INIT", "SUCCESS");

	printf("MEMORY_SIZE : %d\n", heap.maxSize);
	printf("MIN_MEMORY_SIZE : %d\n", heap.minSize);
	printf("MAP_SIZE : %d\n", heap.mapSize);

	printf("Heap : %p\n", &heap);
	printf("Memory : %p - %p\n", heap.memory, heap.memory + heap.maxSize);
	printf("Map : %p - %p\n\n", heap.map, heap.map + heap.mapSize);
	HAL_Delay(1000);

	uint32_t tick = HAL_GetTick();
	uint32_t prev_milis = HAL_GetTick();
	while (1) {
		uint32_t current_milis = HAL_GetTick();
		if ((current_milis - prev_milis) > 1000) {
			printf("###################################################################\n");
			printf("Memory : %p - %p\n", heap.memory, heap.memory + heap.maxSize);
//			StaticBuddyHeap.printMap(heap);
			prev_milis = HAL_GetTick();
			printQueue();
		}
		uint32_t tock = HAL_GetTick();
//		if ((tock - tick) >= 100) {
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xA, tx_bytes1, sizeof(tx_bytes1));
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xB, tx_bytes2, sizeof(tx_bytes2));
//		StaticCanProtocol.addTxMessagePtr(&link1, 0xC, tx_bytes3, sizeof(tx_bytes3));
//		StaticCanProtocol.addTxMessagePtr(&link2, 0xD, tx_bytes4, sizeof(tx_bytes4));
//		StaticCanProtocol.addTxMessagePtr(&link2, 0xE, tx_bytes3, sizeof(tx_bytes3));
//		tick = tock;
//		};
		StaticCanProtocol.thread(&link1);
//		StaticCanProtocol.thread(&link2);
	}
}

struct CanProtocolTest StaticCanProtocolTest = { .canRxInterrupt = canRxInterrupt, .runRx = runRx, .runTx = runTx };

