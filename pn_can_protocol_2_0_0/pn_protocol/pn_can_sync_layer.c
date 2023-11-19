/*
 * pn_can_sync_layer.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */

#include "pn_can_sync_layer.h"
#include "main.h"
#include "crc.h"
#include "stdarg.h"

#define TRANSMIT_TIMEOUT 10000000
#define RECEIVE_TIMEOUT  10000000

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

static uint32_t getMillis() {
	return HAL_GetTick();
}

/**
 * Send thread for transmitting should be called in thread continuously
 * @param link 	: Link where data is to be transmitted
 * @param data	: Data that is to be transmitted
 */
static int txSendThread(SyncLayerCANLink *link, SyncLayerCANData *data) {
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
		if (link->canSend(id, bytes, len)) {
			data->track = SYNC_LAYER_CAN_START_ACK;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_INFO, __func__, "START REQ> 0x%x : 0x%x, %d, %d\n", id,
					*(uint32_t*) bytes, *(uint16_t*) (&bytes[4]),bytes[6]);
#endif
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_WARNING, __func__,
					"START REQ> 0x%x : 0x%x, %d, %d (failed)\n", id,
					*(uint32_t*) bytes, *(uint16_t*) (&bytes[4]),bytes[6]);
#endif

	} else if (data->track == SYNC_LAYER_CAN_DATA) {
		isSuccess = 1;
		id = data->id;
		int indexOffset = data->frameIndex * 7;
		bytes[0] = data->frameIndex;
		int remSize = data->size - indexOffset;
		len = remSize > 7 ? 7 : remSize;
		for (int i = 0; i < len; i++)
			bytes[i + 1] = data->bytes[i + indexOffset];
		if (link->canSend(id, bytes, len + 1)) {
			data->frameIndex++;
			if (remSize <= 7)
				data->track = SYNC_LAYER_CAN_END_REQ;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_INFO, __func__, "DATA> 0x%x : %d, ...\n", id,
					bytes[0]);
#endif
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_WARNING, __func__, "DATA> 0x%x : %d, ...\n", id,
					bytes[0]);
#endif
	} else if (data->track == SYNC_LAYER_CAN_MISSING_DATA) {
		isSuccess = 1;
		id = data->id;
		int indexOffset = data->frameIndex * 7;
		bytes[0] = data->frameIndex;
		int remSize = data->size - indexOffset;
		len = remSize > 7 ? 7 : remSize;
		for (int i = 0; i < len; i++)
			bytes[i + 1] = data->bytes[i + indexOffset];
		if (link->canSend(id, bytes, len + 1)) {
			data->track = SYNC_LAYER_CAN_END_REQ;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_INFO, __func__, "MISSING DATA> 0x%x : %d, ...\n",
					id, bytes[0]);
#endif
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_WARNING, __func__, "MISSING DATA> 0x%x : %d, ...\n",
					id, bytes[0]);
#endif
	} else if (data->track == SYNC_LAYER_CAN_END_REQ) {
		isSuccess = 1;
		len = 8;
		id = link->endReqID;
		*(uint32_t*) bytes = data->id;
		data->crc = crc32_calculate(data->bytes, data->size);
		*(uint32_t*) (&bytes[4]) = data->crc;
		if (link->canSend(id, bytes, len)) {
			data->track = SYNC_LAYER_CAN_END_ACK;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_INFO, __func__, "END REQ> 0x%x : 0x%x, 0x%x \n", id,
					*(uint32_t*) bytes, *(uint32_t*) (&bytes[4]));
#endif
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_WARNING, __func__, "END REQ> 0x%x : 0x%x, 0x%x \n",
					id, *(uint32_t*) bytes, *(uint32_t*) (&bytes[4]));
#endif
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;
	return isSuccess;
}

/**
 * Receive thread for transmitting should be called in thread where CAN data is being received
 * @param link 	: Link where data is to be transmitted
 * @param data	: Data that is to be transmitted
 * @param id	: CAN ID
 * @param bytes	: bytes received from CAN
 * @param len	: length of CAN bytes received
 */
static int txReceiveThread(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
		uint8_t *bytes, uint16_t len) {
	int isSuccess = 0;

	if (data->track == SYNC_LAYER_CAN_START_ACK && id == link->startAckID) {
		isSuccess = 1;
		data->track = SYNC_LAYER_CAN_DATA;
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "START ACK>0x%x : 0x%x, %d, %d\n", id,
				*(uint32_t*) (bytes), *(uint16_t*) (&bytes[4]),bytes[6]);
#endif

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
				data->track = SYNC_LAYER_CAN_TRANSMIT_SUCCESS;
			} else {
#ifdef CONSOLE_ENABLE
				console(CONSOLE_WARNING, __func__, "CRC ERROR>0x%x : 0x%x \n",
						data->id, data->crc);
#endif
				data->numTry--;
				if (data->numTry > 0) {
					data->track = SYNC_LAYER_CAN_START_REQ;
#ifdef CONSOLE_ENABLE
					console(CONSOLE_WARNING, __func__, "RETRY>0x%x : %d\n",
							data->id, data->numTry);
#endif
				} else {
					data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
#ifdef CONSOLE_ENABLE
					console(CONSOLE_ERROR, __func__, "RETRY EXCEEDS>0x%x\n",
							data->id);
#endif
				}
			}
		}
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "END ACK>0x%x : 0x%x, %d, %d, %d \n",
				id, *(uint32_t*) bytes, bytes[4], bytes[5], bytes[6]);
#endif
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;

	if (getMillis() > data->waitTill) {
#ifdef CONSOLE_ENABLE
		console(CONSOLE_WARNING, __func__, "TIMEOUT>0x%x : %d > %d\n", data->id,
		TRANSMIT_TIMEOUT, getMillis() - data->waitTill);
#endif
		data->isTimeOut = 1;
		data->numTry--;
		if (data->numTry > 0) {
			data->track = SYNC_LAYER_CAN_START_REQ;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_WARNING, __func__, "RETRY>0x%x : %d\n", data->id,
					data->numTry);
#endif
		} else {
			data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__, "RETRY EXCEEDS>0x%x\n", data->id);
#endif
		}
	}

	return isSuccess;
}

/**
 * Receive thread for receiving should be called in thread continuously
 * @param link 	: Link where data is to be received
 * @param data	: Data that is to be received
 */
static int rxSendThread(SyncLayerCANLink *link, SyncLayerCANData *data) {
	int isSuccess = 0;
	uint32_t id;
	uint8_t bytes[8];
	if (data->track == SYNC_LAYER_CAN_START_ACK) {
		isSuccess = 1;
		id = link->startAckID;
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) (&bytes[4]) = data->size;
		bytes[6] = data->numTry;
		if (link->canSend(id, bytes, 8)) {
			data->track = SYNC_LAYER_CAN_DATA;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_INFO, __func__, "START ACK> 0x%x : 0x%x, %d, %d\n", id,
					*(uint32_t*) bytes, *(uint16_t*) (&bytes[4]),bytes[6]);
#endif
		}
#ifdef CONSOLE_ENABLE
		else
			console(CONSOLE_WARNING, __func__,
					"START ACK> 0x%x : 0x%x, %d, %d (failed)\n", id,
					*(uint32_t*) bytes, *(uint16_t*) (&bytes[4]),bytes[6]);
#endif
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
			if (link->canSend(id, bytes, 8)) {
				data->track = SYNC_LAYER_CAN_MISSING_DATA;
#ifdef CONSOLE_ENABLE
				console(CONSOLE_INFO, __func__,
						"END ACK> 0x%x : 0x%x, %d, %d, %d\n", id,
						*(uint32_t*) bytes, bytes[4], bytes[5], bytes[6]);
#endif
			}
#ifdef CONSOLE_ENABLE
			else
				console(CONSOLE_WARNING, __func__,
						"END ACK> 0x%x : 0x%x, %d, %d, %d (failed)\n", id,
						*(uint32_t*) bytes, bytes[4], bytes[5], bytes[6]);
#endif
		} else {
			bytes[4] = 0;
			bytes[5] = 0;
			uint32_t crc = crc32_calculate(data->bytes, data->size);
			data->doesCRCMatch = data->crc == crc;
			bytes[6] = data->doesCRCMatch;

			if (link->canSend(id, bytes, 8)) {
				if (data->doesCRCMatch)
					data->track = SYNC_LAYER_CAN_RECEIVE_SUCCESS;
				else {
#ifdef CONSOLE_ENABLE
					console(CONSOLE_WARNING, __func__,
							"CRC ERROR> 0x%x : 0x%x != 0x%x\n", data->id,
							data->crc, crc);
#endif
					data->numTry--;
					if (data->numTry > 0) {
						data->track = SYNC_LAYER_CAN_START_REQ;
#ifdef CONSOLE_ENABLE
						console(CONSOLE_WARNING, __func__, "RETRY>0x%x, %d\n",
								data->id, data->numTry);
#endif
					} else {
						data->track = SYNC_LAYER_CAN_RECEIVE_FAILED;
#ifdef CONSOLE_ENABLE
						console(CONSOLE_ERROR, __func__, "RETRY EXCEEDS>0x%x\n",
								data->id);
#endif
					}
				}
#ifdef CONSOLE_ENABLE
				console(CONSOLE_INFO, __func__,
						"END ACK> 0x%x : 0x%x, %d, %d, %d\n", id,
						*(uint32_t*) bytes, bytes[4], bytes[5], bytes[6]);
#endif
			}
#ifdef CONSOLE_ENABLE
			else
				console(CONSOLE_WARNING, __func__,
						"END ACK> 0x%x : 0x%x, %d, %d, %d (failed)\n", id,
						*(uint32_t*) bytes, bytes[4], bytes[5], bytes[6]);
#endif
		}

	}

	if (isSuccess)
		data->waitTill = getMillis() + RECEIVE_TIMEOUT;

	if (getMillis() > data->waitTill) {
		data->numTry--;
		if (data->numTry > 0) {
			data->track = SYNC_LAYER_CAN_START_REQ;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_WARNING, __func__, "RETRY>0x%x, %d\n", data->id,
					data->numTry);
#endif
		} else {
			data->track = SYNC_LAYER_CAN_RECEIVE_FAILED;
#ifdef CONSOLE_ENABLE
			console(CONSOLE_ERROR, __func__, "RETRY EXCEEDS>0x%x\n", data->id);
#endif
		}
#ifdef CONSOLE_ENABLE
		console(CONSOLE_ERROR, __func__, "TIMEOUT>0x%x : %d > %d\n", data->id,
		RECEIVE_TIMEOUT, getMillis() - data->waitTill);
#endif
	}

	return isSuccess;
}

/**
 * Receive thread for receiving should be called in thread where CAN data is being received
 * @param link 	: Link where data is to be received
 * @param data	: Data that is to be received
 * @param id	: CAN ID
 * @param bytes	: bytes received from CAN
 * @param len	: length of CAN bytes received
 */
static int rxReceiveThread(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
		uint8_t *bytes, uint16_t len) {
	int isSuccess = 0;
	if (data->track == SYNC_LAYER_CAN_START_REQ && id == link->startReqID) {
		for (int i = 0; i < sizeof(data->frameRecords); i++)
			data->frameRecords[i] = 0;
		isSuccess = 1;
		data->size = *(uint16_t*) (&bytes[4]);
		data->track = SYNC_LAYER_CAN_START_ACK;
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "START REQ>0x%x : 0x%x, %d, %d\n", id,
				*(uint32_t*) (bytes), *(uint16_t*) (&bytes[4]),bytes[6]);
#endif
	} else if (id == link->endReqID) {
		isSuccess = 1;
		data->crc = *(uint32_t*) (&bytes[4]);
		data->track = SYNC_LAYER_CAN_END_ACK;
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "END REQ>0x%x : 0x%x, 0x%x\n", id, id,
				*(uint32_t*) bytes, *(uint32_t*) (&bytes));
#endif
	} else if (data->track == SYNC_LAYER_CAN_DATA) {
		isSuccess = 1;
		uint8_t frameIndex = bytes[0];
		int indexOffset = frameIndex * 7;
		int recordIndex = frameIndex / 8;
		int bitIndex = frameIndex % 8;
		data->frameRecords[recordIndex] = data->frameRecords[recordIndex]
				| (1 << bitIndex);
		for (int i = 0; i < (len - 1); i++)
			data->bytes[indexOffset + i] = bytes[i + 1];
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "DATA>0x%x : %d, ...\n", id, bytes[0]);
#endif
	} else if (data->track == SYNC_LAYER_CAN_MISSING_DATA) {
		isSuccess = 1;
		uint8_t frameIndex = bytes[0];
		int indexOffset = frameIndex * 7;
		int recordIndex = frameIndex / 8;
		int bitIndex = frameIndex % 8;
		data->frameRecords[recordIndex] = data->frameRecords[recordIndex]
				| (1 << bitIndex);
		for (int i = 0; i < len - 1; i++)
			data->bytes[indexOffset + i] = bytes[i + 1];
#ifdef CONSOLE_ENABLE
		console(CONSOLE_INFO, __func__, "MISSING DATA>0x%x : %d, ...\n", id,
				bytes[0]);
#endif
	}

	if (isSuccess)
		data->waitTill = getMillis() + TRANSMIT_TIMEOUT;

	return isSuccess;
}

struct SyncLayerCanControl StaticSyncLayerCan = {
	.rxReceiveThread = 	rxReceiveThread,
	.rxSendThread = rxSendThread,
	.txReceiveThread = txReceiveThread,
	.txSendThread = txSendThread
};

