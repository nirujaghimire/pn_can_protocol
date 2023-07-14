/*
 * pn_can_sync_layer_test.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NIRUJA
 */

#include "pn_can_sync_layer.h"
#include "main.h"

extern CRC_HandleTypeDef hcrc;

static SyncLayerCanLink link1 = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7 };
static SyncLayerCanLink link2 = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

static SyncLayerCanData data1;
static SyncLayerCanData data2;
static SyncLayerCanData data3;

static uint8_t tx_bytes1[16] = { 1, 2, 3, 4, [9]=10 };
static uint8_t tx_bytes2[16] = { 1, 5, 3, 7};
static uint8_t tx_bytes3[16] = { 2, 4, 6, 8};

static uint8_t rx_bytes1[16];
static uint8_t rx_bytes2[16];
static uint8_t rx_bytes3[16];

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
static void canRxInterruptRx() {
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, bytes);
//	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
//	for (int i = 0; i < rx_header.DLC; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");

	uint32_t id = *(uint32_t*) bytes;
	if (id == data1.id || data1.id == rx_header.ExtId) {
		StaticSyncLayerCan.rxReceiveThread(&link1, &data1, rx_header.ExtId,
				bytes, rx_header.DLC);
	} else if (id == data2.id || data2.id == rx_header.ExtId) {
		StaticSyncLayerCan.rxReceiveThread(&link1, &data2, rx_header.ExtId,
				bytes, rx_header.DLC);
	} else if (id == data3.id || data3.id == rx_header.ExtId) {
		StaticSyncLayerCan.rxReceiveThread(&link2, &data3, rx_header.ExtId,
				bytes, rx_header.DLC);
	}

}

static void canRxInterruptTx() {
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, bytes);
//	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
//	for (int i = 0; i < rx_header.DLC; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");

	uint32_t id = *(uint32_t*) bytes;
	if (id == data1.id || data1.id == rx_header.ExtId) {
		StaticSyncLayerCan.txReceiveThread(&link1, &data1, rx_header.ExtId,
				bytes, rx_header.DLC);
	}else if (id == data2.id || data2.id == rx_header.ExtId) {
		StaticSyncLayerCan.txReceiveThread(&link1, &data2, rx_header.ExtId,
				bytes, rx_header.DLC);
	}else if (id == data3.id || data3.id == rx_header.ExtId) {
		StaticSyncLayerCan.txReceiveThread(&link2, &data3, rx_header.ExtId,
				bytes, rx_header.DLC);
	}

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

void txCallback(SyncLayerCanLink *link, SyncLayerCanData *data, uint8_t status) {
	if (!status) {
		printf("Data failed\n");
		data->track = SYNC_LAYER_CAN_START_REQUEST;
		return;
	}
	printf("ID : 0x%0x", (int) data->id);
	for (int i = 0; i < data->size; i++)
		printf(" %d ", data->bytes[i]);
	printf("status : %d\n", status);
	data->track = SYNC_LAYER_CAN_START_REQUEST;
}

void rxCallback(SyncLayerCanLink *link, SyncLayerCanData *data, uint8_t status) {
	if (!status) {
		printf("Data failed\n");
		data->track = SYNC_LAYER_CAN_START_REQUEST;
		return;
	}

	printf("ID : 0x%0x", (int) data->id);
	for (int i = 0; i < data->size; i++)
		printf(" %d ", data->bytes[i]);
	printf("status : %d\n", status);
	data->track = SYNC_LAYER_CAN_START_REQUEST;
}

static void runRx() {
	canInit();

	data1.id = 0xA;
	data1.bytes = rx_bytes1;
	data1.size = sizeof(rx_bytes1);
	data1.track = SYNC_LAYER_CAN_START_REQUEST;
	data1.data_retry = 0;
	data1.dynamically_alocated = 0;

	data2.id = 0xB;
	data2.bytes = rx_bytes2;
	data2.size = sizeof(rx_bytes2);
	data2.track = SYNC_LAYER_CAN_START_REQUEST;
	data2.data_retry = 0;
	data2.dynamically_alocated = 0;

	data3.id = 0xC;
	data3.bytes = rx_bytes3;
	data3.size = sizeof(rx_bytes3);
	data3.track = SYNC_LAYER_CAN_START_REQUEST;
	data3.data_retry = 0;
	data3.dynamically_alocated = 0;

	console("\n\nSOURCE INIT", "SUCCESS");

	while (1) {
		StaticSyncLayerCan.rxSendThread(&link1, &data1, canSend, rxCallback);
		StaticSyncLayerCan.rxSendThread(&link1, &data2, canSend, rxCallback);
		StaticSyncLayerCan.rxSendThread(&link2, &data3, canSend, rxCallback);

	}
}

static void runTx() {
	canInit();

	data1.id = 0xA;
	data1.bytes = tx_bytes1;
	data1.size = sizeof(tx_bytes1);
	data1.track = SYNC_LAYER_CAN_START_REQUEST;
	data1.data_retry = 0;
	data1.dynamically_alocated = 0;

	data2.id = 0xB;
	data2.bytes = tx_bytes2;
	data2.size = sizeof(tx_bytes2);
	data2.track = SYNC_LAYER_CAN_START_REQUEST;
	data2.data_retry = 0;
	data2.dynamically_alocated = 0;

	data3.id = 0xC;
	data3.bytes = tx_bytes3;
	data3.size = sizeof(tx_bytes3);
	data3.track = SYNC_LAYER_CAN_START_REQUEST;
	data3.data_retry = 0;
	data3.dynamically_alocated = 0;

	console("\n\nSOURCE INIT", "SUCCESS");

	while (1) {
		StaticSyncLayerCan.txSendThread(&link1, &data1, canSend, txCallback);
		StaticSyncLayerCan.txSendThread(&link1, &data2, canSend, txCallback);
		StaticSyncLayerCan.txSendThread(&link2, &data3, canSend, txCallback);
	}
}

struct SyncLayerCanTest StaticSyncLayerCanTest = { .canRxInterruptRx =
		canRxInterruptRx, .canRxInterruptTx = canRxInterruptTx, .runRx = runRx,
		.runTx = runTx };
