/*
 * pn_can_protocol_test.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "main.h"
#include "pn_can_protocol.h"

#define TEST_ENABLE
#define TX_ENABLE

#ifdef TEST_ENABLE
CANLink *canLink;

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
	StaticCANLink.canReceive(canLink, rx_header.ExtId, bytes, rx_header.DLC);
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

uint8_t dataBytes1[20];
uint8_t dataBytes2[40];
uint8_t dataBytes3[60];
uint8_t buffer[4096 + mapSize(4096, 8)];
#ifdef TX_ENABLE
void run() {
	canInit();
	for (int i = 0; i < sizeof(dataBytes1); i++)
		dataBytes1[i] = i;
	for (int i = 0; i < sizeof(dataBytes2); i++)
		dataBytes2[i] = i;
	for (int i = 0; i < sizeof(dataBytes3); i++)
		dataBytes3[i] = i;

	printf("----------------------TX INITIATING-----------------------\n");
	HAL_Delay(1000);

	BuddyHeap heap = StaticBuddyHeap.new(buffer,sizeof(buffer),8);
	canLink = StaticCANLink.new(0x1, 0x2, 0x3, 0x4, canSend, txCallback, rxCallback, &heap,0);

	uint32_t prevMillis = HAL_GetTick();
	while (1) {
		if((HAL_GetTick()-prevMillis)>100){
			StaticCANLink.addTxMsgPtr(canLink, 0xA, dataBytes1, sizeof(dataBytes1));
			StaticCANLink.addTxMsgPtr(canLink, 0xB, dataBytes2, sizeof(dataBytes2));
			StaticCANLink.addTxMsgPtr(canLink, 0xC, dataBytes3, sizeof(dataBytes3));
			prevMillis = HAL_GetTick();
		}
		StaticCANLink.thread(canLink);
//		HAL_Delay(1000);
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
	canLink = StaticCANLink.new(0x1, 0x2, 0x3, 0x4, canSend, txCallback,
			rxCallback, &heap, 0);

//	uint32_t prevMillis = HAL_GetTick();
	while (1) {
//		if((HAL_GetTick()-prevMillis)>10000){
//			StaticCANLink.addTxMsg(canLink, 0xA1, dataBytes, sizeof(dataBytes));
//			prevMillis = HAL_GetTick();
//		}

		StaticCANLink.thread(canLink);
//		HAL_Delay(100);
	}
}
#endif

#endif
