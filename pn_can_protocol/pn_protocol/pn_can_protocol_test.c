/*
 * pn_can_protocol_test.c
 *
 *  Created on: Jul 10, 2023
 *      Author: NIRUJA
 */

#include "pn_can_protocol.h"
#include "main.h"

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
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING)
			!= HAL_OK)
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
//	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
//	for (int i = 0; i < rx_header.DLC; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");
	StaticCanProtocol.recThread(&link1, rx_header.ExtId, bytes, rx_header.DLC);
	StaticCanProtocol.recThread(&link2, rx_header.ExtId, bytes, rx_header.DLC);

}

static CAN_TxHeaderTypeDef tx_header;
static uint32_t tx_mailbox;
static uint8_t canSend(uint32_t id, uint8_t *bytes, uint8_t len) {

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

/**********************MAIN THREAD****************************/
static uint8_t txCallback1(uint32_t id, uint8_t *bytes, uint16_t size,
		uint8_t status) {
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

static uint8_t rxCallback1(uint32_t id, uint8_t *bytes, uint16_t size,
		uint8_t status) {
	printf("Rx Data1 : ");
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

static uint8_t txCallback2(uint32_t id, uint8_t *bytes, uint16_t size,
		uint8_t status) {
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

static uint8_t rxCallback2(uint32_t id, uint8_t *bytes, uint16_t size,
		uint8_t status) {
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

static uint8_t tx_bytes1[16] = { 1, 2, 3, 4, [9]=10 };
static uint8_t tx_bytes2[16] = { 1, 5, 3, 7 };
static uint8_t tx_bytes3[16] = { 2, 4, 6, 8 };

static void runRx() {
	StaticCanProtocol.addLink(&link1, canSend, txCallback1, rxCallback1, 1);
	StaticCanProtocol.addLink(&link2, canSend, txCallback2, rxCallback2, 1);
	canInit();

	console("\n\nSOURCE INIT", "SUCCESS");
	HAL_Delay(1000);

	uint32_t tick = HAL_GetTick();
	while (1) {
		uint32_t tock = HAL_GetTick();
		if ((tock - tick) >= 10) {
			StaticCanProtocol.addTxMessagePtr(&link1, 0xA1, tx_bytes1,
					sizeof(tx_bytes1));
			StaticCanProtocol.addTxMessagePtr(&link1, 0xB1, tx_bytes2,
					sizeof(tx_bytes2));
			StaticCanProtocol.addTxMessagePtr(&link1, 0xC1, tx_bytes3,
					sizeof(tx_bytes3));

			tick = tock;
		}

		StaticCanProtocol.sendThread(&link1);
		StaticCanProtocol.sendThread(&link2);
	}
}

static void runTx() {
	StaticCanProtocol.addLink(&link1, canSend, txCallback1, rxCallback1, 1);
	StaticCanProtocol.addLink(&link2, canSend, txCallback2, rxCallback2, 1);
	canInit();

	console("\n\nSOURCE INIT", "SUCCESS");
	HAL_Delay(1000);

	uint32_t tick = HAL_GetTick();
	while (1) {
		uint32_t tock = HAL_GetTick();
		if ((tock - tick) >= 10) {
			StaticCanProtocol.addTxMessagePtr(&link1, 0xA, tx_bytes1,
					sizeof(tx_bytes1));
			StaticCanProtocol.addTxMessagePtr(&link1, 0xB, tx_bytes2,
					sizeof(tx_bytes2));
			StaticCanProtocol.addTxMessagePtr(&link1, 0xC, tx_bytes3,
					sizeof(tx_bytes3));
			StaticCanProtocol.addTxMessagePtr(&link2, 0xD, tx_bytes3,
					sizeof(tx_bytes3));
			StaticCanProtocol.addTxMessagePtr(&link2, 0xE, tx_bytes3,
					sizeof(tx_bytes3));
			tick = tock;


		}
		StaticCanProtocol.sendThread(&link1);
		StaticCanProtocol.sendThread(&link2);
	}
}

struct CanProtocolTest StaticCanProtocolTest = { .canRxInterrupt =
		canRxInterrupt, .runRx = runRx, .runTx = runTx };

