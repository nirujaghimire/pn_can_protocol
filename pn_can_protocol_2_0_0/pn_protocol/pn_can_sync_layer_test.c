/*
 * pn_can_sync_layer_test.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */

#include "main.h"
#include "crc.h"

#define TRANSMIT_TIMEOUT 1000
#define RECEIVE_TIMEOUT 1000

typedef enum {
	SYNC_LAYER_CAN_START_REQ,
	SYNC_LAYER_CAN_START_ACK,
	SYNC_LAYER_CAN_DATA,
	SYNC_LAYER_CAN_MISSING_DATA,
	SYNC_LAYER_CAN_END_REQ,
	SYNC_LAYER_CAN_END_ACK,
	/////////////////////////
	SYNC_LAYER_CAN_TRANSMIT_SUCCESS,
	SYNC_LAYER_CAN_TRANSMIT_FAILED,
	SYNC_LAYER_CAN_RECEIVE_SUCCESS,
	SYNC_LAYER_CAN_RECEIVE_FAILED
} SyncLayerCANTrack;

typedef struct {
	uint32_t id;
	uint8_t *bytes;
	uint16_t size;
	uint8_t numTry;
	///////////////////////
	uint8_t frameIndex;
	//////////////////////
	uint8_t frameRecords[19];
	///////////////////////
	uint32_t crc;
	SyncLayerCANTrack track;
	uint8_t doesCRCMatch;
	uint32_t waitTill;
	uint8_t isTimeOut;

} SyncLayerCANData;

typedef struct {
	uint32_t startReqID;
	uint32_t startAckID;
	uint32_t endReqID;
	uint32_t endAckID;
/////////////////////////
//	int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len);
//	void (*txCallback)(SyncLayerCANLink *link,SyncLayerCANData *data, uint8_t status);

} SyncLayerCANLink;

static void console(const char *title, const char *msg) {
	printf("%s:: %s\n", title, msg);
}

static uint32_t getMillis() {
	return HAL_GetTick();
}

SyncLayerCANLink link;
SyncLayerCANData data;
static int canSend(uint32_t id, uint8_t *bytes, uint8_t len);

int txSendThread(SyncLayerCANLink *link, SyncLayerCANData *data) {
	uint32_t id;
	uint8_t bytes[8];
	uint16_t len;
	int isSuccess = 0;
	if (data->track == SYNC_LAYER_CAN_START_REQ) {
		data->doesCRCMatch = 1;
		data->isTimeOut = 0;
		isSuccess = 1;
		data->frameIndex = 0;
		id = link->startReqID;
		*(uint32_t*) (bytes) = data->id;
		*(uint16_t*) (bytes + 4) = data->size;
		bytes[6] = data->numTry;
		len = 8;
		if (canSend(id, bytes, len))
			data->track = SYNC_LAYER_CAN_START_ACK;
	} else if (data->track == SYNC_LAYER_CAN_DATA) {
		isSuccess = 1;
		id = data->id;
		int index = data->frameIndex * 7;
		bytes[0] = data->frameIndex;
		int remSize = data->size - index;
		len = remSize > 7 ? 7 : remSize;
		for (int i = 0; i < len; i++)
			bytes[i + 1] = data->bytes[i + index]; //TODO :
		if (canSend(id, bytes, len + 1)) {
			data->frameIndex++;
			if (remSize <= 7)
				data->track = SYNC_LAYER_CAN_END_REQ;
		}
	} else if (data->track == SYNC_LAYER_CAN_MISSING_DATA) {
		isSuccess = 1;
		id = data->id;
		int index = data->frameIndex * 7;
		bytes[0] = data->frameIndex;
		int remSize = data->size - index;
		len = remSize > 7 ? 7 : remSize;
		for (int i = 0; i < len; i++)
			bytes[i + 1] = data->bytes[i + index]; //TODO :
		if (canSend(id, bytes, len + 1))
			data->track = SYNC_LAYER_CAN_END_REQ;
	} else if (data->track == SYNC_LAYER_CAN_END_REQ) {
		isSuccess = 1;
		len = 8;
		id = link->endReqID;
		*(uint32_t*) bytes = data->id;
		data->crc = crc32_calculate(data->bytes, data->size);
		*(uint32_t*) (&bytes[4]) = data->crc;
		if (canSend(id, bytes, len))
			data->track = SYNC_LAYER_CAN_END_ACK;
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;
	return isSuccess;
}

int txReceiveThread(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
		uint8_t *bytes, uint16_t len) {
	int isSuccess = 0;

	if (data->track == SYNC_LAYER_CAN_START_ACK && id == link->startAckID) {
		isSuccess = 1;
		data->track = SYNC_LAYER_CAN_DATA;
	} else if (data->track == SYNC_LAYER_CAN_END_ACK && id == link->endAckID) {
		isSuccess = 1;
		uint8_t missingDataAvailable = bytes[4];
		uint8_t missingFrameIndex = bytes[5];
		data->doesCRCMatch = bytes[6];
		if (missingDataAvailable) {
			data->frameIndex = missingFrameIndex;
			data->track = SYNC_LAYER_CAN_MISSING_DATA;
		} else {
			if (data->doesCRCMatch) {
				data->track = SYNC_LAYER_CAN_TRANSMIT_SUCCESS; //TODO
			} else {
				data->numTry--;
				if (data->numTry > 0)
					data->track = SYNC_LAYER_CAN_START_REQ;
				else
					data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
			}
		}
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;

	if (getMillis() > data->waitTill) {
		data->isTimeOut = 1;
		data->numTry--;
		if (data->numTry > 0)
			data->track = SYNC_LAYER_CAN_START_REQ;
		else
			data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
	}

	return isSuccess;
}

int rxSendThread(SyncLayerCANLink *link, SyncLayerCANData *data) {
	int isSuccess = 0;
	uint32_t id;
	uint8_t bytes[8];
	if (data->track == SYNC_LAYER_CAN_START_ACK) {
		isSuccess = 1;
		id = link->startAckID;
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) (&bytes[4]) = data->size;
		bytes[6] = data->numTry;
		if (canSend(id, bytes, 8))
			data->track = SYNC_LAYER_CAN_DATA;

	} else if (data->track == SYNC_LAYER_CAN_END_ACK) {
		isSuccess = 1;
		id = link->endAckID;
		*(uint32_t*) bytes = data->id;

		uint8_t missingDataAvailable = 0;
		int missingFrameIndex = 0;
		int totalFrame = data->size / 7 + (data->size % 7 != 0);
		int totalGroupFrame = totalFrame / 8 + (totalFrame % 8 != 0);
		for (int group = 0; group < totalGroupFrame; group++) {
			uint8_t record = data->frameRecords[group];
			int remFrame = totalFrame - (group * 8);
			int bitLen = (remFrame > 8) ? 8 : remFrame;
			for (int bit = 0; bit < bitLen; bit++) {
				if (!(record & (1 << bit))) {
					missingDataAvailable = 1;
					missingFrameIndex = bit + 8 * group;
					break;
				}
			}
			if (missingDataAvailable)
				break;
		}

		if (missingDataAvailable) {
			bytes[4] = 1;
			bytes[5] = missingFrameIndex;
			if (canSend(id, bytes, 8))
				data->track = SYNC_LAYER_CAN_MISSING_DATA;
		} else {
			bytes[4] = 0;
			bytes[5] = 0;
			data->doesCRCMatch = data->crc
					== crc32_calculate(data->bytes, data->size);
			bytes[6] = data->doesCRCMatch;
			if (canSend(id, bytes, 8)) {
				if (data->doesCRCMatch)
					data->track = SYNC_LAYER_CAN_RECEIVE_SUCCESS; //TODO
				else
					data->track = SYNC_LAYER_CAN_RECEIVE_FAILED; //TODO
			}
		}

	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;

	if (getMillis() > data->waitTill)
		data->track = SYNC_LAYER_CAN_RECEIVE_FAILED;

	return isSuccess;
}

int rxReceiveThread(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
		uint8_t *bytes, uint16_t len) {
	int isSuccess = 0;
	if (data->track == SYNC_LAYER_CAN_START_REQ && id == link->startReqID) {
		for (int i = 0; i < 19; i++)
			data->frameRecords[i] = 0;
		isSuccess = 1;
		data->track = SYNC_LAYER_CAN_START_ACK;
	} else if (data->track == SYNC_LAYER_CAN_DATA) {
		isSuccess = 1;
		uint8_t frameIndex = bytes[0];
		int indexOffset = frameIndex * 7;
		int recordIndex = frameIndex / 8;
		int bitIndex = frameIndex % 8;
		data->frameRecords[recordIndex] = data->frameRecords[recordIndex]
				| (1 << bitIndex);
		for (int i = 0; i < 7; i++)
			data->bytes[indexOffset + i] = bytes[i + 1];

	} else if (data->track == SYNC_LAYER_CAN_MISSING_DATA) {
		isSuccess = 1;
		uint8_t frameIndex = bytes[0];
		int indexOffset = frameIndex * 7;
		int recordIndex = frameIndex / 8;
		int bitIndex = frameIndex % 8;
		data->frameRecords[recordIndex] = data->frameRecords[recordIndex]
				| (1 << bitIndex);
		for (int i = 0; i < 7; i++)
			data->bytes[indexOffset + i] = bytes[i + 1];
	} else if (id == link->endReqID) {
		isSuccess = 1;
		data->crc = *(uint32_t*) (&bytes[4]);
		data->track = SYNC_LAYER_CAN_END_ACK;
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;

	return isSuccess;
}

////////////////////////////////////////////////////////////////////////

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
void canRxInterrupt() {
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, bytes);
//	printf("Interrupt-> 0x%02x : ", (unsigned int) rx_header.ExtId);
//	for (int i = 0; i < rx_header.DLC; ++i)
//		printf("%d ", bytes[i]);
//	printf("\n");
	txReceiveThread(&link, &data, rx_header.ExtId, bytes, rx_header.DLC);

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

void txCallback(SyncLayerCANLink *link, SyncLayerCANData *data, uint8_t status) {
	if (!status) {
		printf("Data failed\n");
		data->track = SYNC_LAYER_CAN_START_REQ;
		return;
	}
	printf("ID : 0x%0x", (int) data->id);
	for (int i = 0; i < data->size; i++)
		printf(" %d ", data->bytes[i]);
	printf("status : %d\n", status);
	data->track = SYNC_LAYER_CAN_START_REQ;
}

uint8_t tx_bytes[8];
void runTx() {
	canInit();

	for (int i = 0; i < sizeof(tx_bytes); i++)
		tx_bytes[i] = i;

/////////link////////////
	link.startReqID = 0x1;
	link.startAckID = 0x2;
	link.endReqID = 0x3;
	link.endAckID = 0x4;

//	link.canSend = canSend;
//	link.txCallback = txCallback;

/////////data//////////////
	data.id = 0xA;
	data.bytes = tx_bytes;
	data.size = sizeof(tx_bytes);
	data.numTry = 3;
	data.track = SYNC_LAYER_CAN_START_REQ;

	while (1) {
		txSendThread(&link, &data);

	}
}
