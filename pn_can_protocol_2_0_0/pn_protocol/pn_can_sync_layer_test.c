/*
 * pn_can_sync_layer_test.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "main.h"
#include "pn_can_sync_layer.h"

SyncLayerCANLink link;
SyncLayerCANData data;


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
	StaticSyncLayerCan.txReceiveThread(&link, &data, rx_header.ExtId, bytes, rx_header.DLC);
//	StaticSyncLayerCan.rxReceiveThread(&link, &data, rx_header.ExtId, bytes, rx_header.DLC);

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

/////////link////////////
	link.startReqID = 0x1;
	link.startAckID = 0x2;
	link.endReqID = 0x3;
	link.endAckID = 0x4;
	link.canSend = canSend;

/////////data//////////////
	data.id = 0xA;
	data.bytes = tx_bytes;
	data.size = sizeof(tx_bytes);
	data.numTry = 2;
	data.track = SYNC_LAYER_CAN_START_REQ;
	data.waitTill = HAL_MAX_DELAY;

	printf("----------------------INITIATING-----------------------\n");
	HAL_Delay(1000);

	while (1) {
		if (data.track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS) {
			txCallback(data.id, data.bytes, data.size, 1);
			data.track = SYNC_LAYER_CAN_START_REQ;
		} else if (data.track == SYNC_LAYER_CAN_TRANSMIT_FAILED) {
			txCallback(data.id, data.bytes, data.size, 0);
			data.track = SYNC_LAYER_CAN_START_REQ;
		}

		StaticSyncLayerCan.txSendThread(&link, &data);
//		HAL_Delay(1000);
	}
}


void rxCallback(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status) {
	printf("RX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);

	if (!status) {
		printf("(failed)\n");
		return;
	}
	printf("\n");

}

uint8_t rx_bytes[70];
void runRx() {
	canInit();

//	for (int i = 0; i < sizeof(rx_bytes); i++)
//		rx_bytes[i] = i;

/////////link////////////
	link.startReqID = 0x1;
	link.startAckID = 0x2;
	link.endReqID = 0x3;
	link.endAckID = 0x4;

	link.canSend = canSend;
//	link.txCallback = txCallback;

/////////data//////////////
	data.id = 0xA;
	data.bytes = rx_bytes;
	data.size = sizeof(rx_bytes);
	data.numTry = 3;
	data.track = SYNC_LAYER_CAN_START_REQ;
	data.waitTill = HAL_MAX_DELAY;

	printf("-------------------INITIATING-------------------\n");
	HAL_Delay(1000);

	while (1) {
		StaticSyncLayerCan.rxSendThread(&link, &data);
		if (data.track == SYNC_LAYER_CAN_RECEIVE_SUCCESS) {
			rxCallback(data.id, data.bytes, data.size, 1);
			data.track = SYNC_LAYER_CAN_START_REQ;
		} else if (data.track == SYNC_LAYER_CAN_RECEIVE_FAILED) {
			rxCallback(data.id, data.bytes, data.size, 0);
			data.track = SYNC_LAYER_CAN_START_REQ;
		}
//		HAL_Delay(1000);
	}
}
