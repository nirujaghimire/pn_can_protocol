/*
 * pn_can_sync_layer.c
 *
 *  Created on: Jul 7, 2023
 *      Author: NIRUJA
 */

#include "pn_can_sync_layer.h"
#include "stdarg.h"
#include "main.h"
#include "crc.h"
#include "test.h"


#define SYNC_LAYER_CAN_TRANSMIT_TIMEOUT 5000
#define SYNC_LAYER_CAN_RECEIVE_TIMEOUT 10000

#define SYNC_LAYER_CAN_TX_SEND_RETRY 3

/******************CONSOLE*****************************/
typedef enum {
	CONSOLE_ERROR, CONSOLE_INFO, CONSOLE_WARNING
} ConsoleStatus;
static void console(ConsoleStatus status, const char *func_name,
		const char *msg, ...) {
	if (status == CONSOLE_INFO||status==CONSOLE_WARNING)
		return;
	//TODO make naked and show all registers
	if (status == CONSOLE_ERROR) {
		RED;
		printf("pn_can_sync_layer.c|%s> ERROR :", func_name);
	} else if (status == CONSOLE_INFO) {
		GREEN;
		printf("pn_can_sync_layer.c|%s> INFO : ", func_name);
	} else if (status == CONSOLE_WARNING) {
		YELLOW;
		printf("pn_can_sync_layer.c|%s> WARNING : ", func_name);
	} else {
		printf("pn_can_sync_layer.c|%s: ", func_name);
	}
	va_list args;
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
	RESET;
}

/*****************PRIVATE COMMON**********************/
static uint32_t timeInMillis() {
	return HAL_GetTick();
}

static uint32_t getCRCValue(uint8_t *buffer, uint32_t len) {
	return crc32_calculate(buffer, len);
}

/******************TRANSMIT***************************/
/*
 * This should be called in thread or timer periodically
 * @param link 		: Link where data is going to be transmitted
 * @param data		: Sync Layer Can Data
 * @param txCallback: This is callback called if data transmitted successfully or failed to transmit data
 * @return			: 1 txCallback is called 0 for no callback called
 */
static uint8_t txSendThread(SyncLayerCanLink *link,
		SyncLayerCanData *data,
		uint8_t (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
		void (*txCallback)(SyncLayerCanLink *link, SyncLayerCanData *data,
				uint8_t status)) {
	uint8_t bytes[8] = { 0 };
	SyncLayerCanTrack prev_track = data->track;

	/* Check success */
	if (data->track == SYNC_LAYER_CAN_TRANSMIT_SUCCESS) {
		txCallback(link, data, 1);
		return 1;
	} else if (data->track == SYNC_LAYER_CAN_TRANSMIT_FAILED) {
		if (data->data_retry > SYNC_LAYER_CAN_TX_SEND_RETRY) {
			/* Retry exceeds limit */
			console(CONSOLE_ERROR, __func__, "Sending failed exceed limit %d\n",
					data->data_retry);
			txCallback(link, data, 0);
			return 1;
		} else {
			/* Retry available */
			console(CONSOLE_WARNING, __func__,
					"Retrying to send data 0x%0x for %d\n", data->id,
					data->data_retry);
			data->track = SYNC_LAYER_CAN_START_REQUEST;
			data->data_retry++;
		}
	}

	/* Check Track */
	if (data->track == SYNC_LAYER_CAN_START_REQUEST) {
		/* START REQ */

		data->count = 0;
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) ((uint32_t*) bytes + 1) = data->size;
		if (!canSend(link->start_req_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_WARNING, __func__,
					"Start request 0x%0x of data 0x%0x send failed\n",
					link->start_req_ID, data->id);
		} else {
			/* Can sending success */
			data->track = SYNC_LAYER_CAN_START_ACK;
			console(CONSOLE_INFO, __func__,
					"Start request 0x%0x of data 0x%0x send successful\n",
					link->start_req_ID, data->id);
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA) {
		/* DATA */

		/* Calculate new data size can be transmitted */
		uint16_t new_data_size = data->size - data->count;
		if (new_data_size >= 8)
			new_data_size = 8;

		if (!canSend(data->id, data->bytes + data->count, new_data_size)) {
			/* Can sending failed */
			console(CONSOLE_WARNING, __func__,
					"Data (%d-%d) of data 0x%0x send failed\n", data->count,
					new_data_size, data->id);
		} else {
			/* Can sending success */
			data->track = SYNC_LAYER_CAN_DATA_ACK;
			data->count += new_data_size;
			console(CONSOLE_INFO, __func__,
					"Data (%d-%d) of data 0x%0x send success\n", data->count,
					new_data_size, data->id);
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA_COUNT_RESET_REQUEST) {
		/* DATA COUNT RESET REQ */

		*(uint32_t*) bytes = data->id;
		*(uint16_t*) ((uint32_t*) bytes + 1) = data->count;
		if (!canSend(link->data_count_reset_req_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_WARNING, __func__,
					"Data count %d reset request 0x%0x of data 0x%0x send failed\n",
					data->count, link->data_count_reset_req_ID, data->id);
		} else {
			/* Can sending success */
			data->track = SYNC_LAYER_CAN_DATA_COUNT_RESET_ACK;
			console(CONSOLE_INFO, __func__,
					"Data count %d reset request 0x%0x of data 0x%0x send success\n",
					data->count, link->data_count_reset_req_ID, data->id);
		}
	} else if (data->track == SYNC_LAYER_CAN_END_REQUEST) {
		/* END REQ */

		*(uint32_t*) bytes = data->id;
		data->crc = getCRCValue(data->bytes, data->size); //Calculate crc

		*((uint32_t*) bytes + 1) = data->crc;
		if (!canSend(link->end_req_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_WARNING, __func__,
					"Data end request request 0x%0x of data 0x%0x send failed\n",
					link->data_count_reset_req_ID, data->id);
		} else {
			/* Can sending success */
			data->track = SYNC_LAYER_CAN_END_ACK;
			console(CONSOLE_INFO, __func__,
					"Data end request request 0x%0x of data 0x%0x send success\n",
					link->data_count_reset_req_ID, data->id);
		}
	}

	if (prev_track != data->track) {
		data->time_elapse = timeInMillis();
		if (data->time_elapse == 0)
			console(CONSOLE_WARNING, __func__, "Time elapse %d\n",
					data->time_elapse);
	}

	/* Check transmit timeout */
	if ((timeInMillis() - data->time_elapse) > SYNC_LAYER_CAN_TRANSMIT_TIMEOUT) {
		console(CONSOLE_WARNING, __func__, "Data transmit 0x%0x timeout %d\n",
				data->id, SYNC_LAYER_CAN_TRANSMIT_TIMEOUT);
		data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
	}

	return 0;
}

/*
 * This should be called in thread or timer periodically
 * @param link 			: Link where data is going to be transmitted
 * @param data			: Sync Layer Can Data
 * @param can_id		: Can ID received
 * @param can_bytes		: Can bytes received
 * @param can_bytes_len	: Can bytes length
 */
static void txReceiveThread(SyncLayerCanLink *link,
		SyncLayerCanData *data, uint32_t can_id, uint8_t *can_bytes,
		uint8_t can_bytes_len) {
	uint32_t data_id;

	if (data->track == SYNC_LAYER_CAN_START_ACK
			&& can_id == link->start_ack_ID) {
		/* START ACK */
		data_id = *(uint32_t*) can_bytes;
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"Start ack 0x%0x of data 0x%0x contains wrong data ID 0x%0x\n",
					link->start_ack_ID, data->id, data_id);
		} else {
			/* ID matched */
			console(CONSOLE_INFO, __func__,
					"Start ack 0x%0x of data 0x%0x success\n",
					link->start_ack_ID, data->id);
			data->track = SYNC_LAYER_CAN_DATA;
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA_ACK
			&& can_id == link->data_ack_ID) {
		/* DATA ACK */
		data_id = *(uint32_t*) can_bytes;
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"Data ack 0x%0x of data 0x%0x contains wrong data ID 0x%0x\n",
					link->data_ack_ID, data->id, data_id);
		} else {
			/* ID matched */
			uint16_t data_count = *(uint16_t*) ((uint32_t*) can_bytes + 1);
			if (data_count > data->count) {
				/* Destination count is greater than actual */
				console(CONSOLE_WARNING, __func__,
						"Data ack 0x%0x of data 0x%0x contains higher count %d then actual %d\n",
						link->data_ack_ID, data->id, data_count, data->count);
				data->track = SYNC_LAYER_CAN_DATA_COUNT_RESET_REQUEST;
			} else if (data_count < data->count) {
				/* Destination count is smaller than actual */
				console(CONSOLE_WARNING, __func__,
						"Data ack 0x%0x of data 0x%0x contains lower count %d then actual %d\n",
						link->data_ack_ID, data->id, data_count, data->count);

				console(CONSOLE_WARNING, __func__,
						"Count %d of data 0x%0x is reset to %d\n", data->count,
						data->id, data_count, data_count);
				data->count = data_count;
				data->track = SYNC_LAYER_CAN_DATA_COUNT_RESET_REQUEST;
			} else {
				/* Destination count and source count is equal */
				console(CONSOLE_INFO, __func__,
						"Data ack 0x%0x of data 0x%0x success\n",
						link->data_ack_ID, data->id);
				if (data->count < data->size)
					data->track = SYNC_LAYER_CAN_DATA;
				else
					data->track = SYNC_LAYER_CAN_END_REQUEST;
			}
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA_COUNT_RESET_ACK
			&& can_id == link->data_count_reset_ack_ID) {
		/* DATA COUNT RESET ACK */
		data_id = *(uint32_t*) can_bytes;
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"Data count reset ack 0x%0x of data 0x%0x contains wrong data ID 0x%0x\n",
					link->data_count_reset_ack_ID, data->id, data_id);
		} else {
			/* ID matched */
			console(CONSOLE_INFO, __func__,
					"Data count reset ack 0x%0x of data 0x%0x success\n",
					link->start_ack_ID, data->id);
			data->track = SYNC_LAYER_CAN_DATA;
		}
	} else if (data->track == SYNC_LAYER_CAN_END_ACK
			&& can_id == link->end_ack_ID) {
		/* END ACK */
		data_id = *(uint32_t*) can_bytes;
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"Data count reset ack 0x%0x of data 0x%0x contains wrong data ID 0x%0x\n",
					link->end_ack_ID, data->id, data_id);
		} else {
			/* ID matched */
			console(CONSOLE_INFO, __func__,
					"Data count reset ack 0x%0x of data 0x%0x success\n",
					link->end_ack_ID, data->id);
			uint8_t is_transmit_success =
					*(uint8_t*) ((uint32_t*) can_bytes + 1);
			if (is_transmit_success) {
				console(CONSOLE_INFO, __func__, "Data CRC match success\n");
				data->track = SYNC_LAYER_CAN_TRANSMIT_SUCCESS;
			} else {
				console(CONSOLE_WARNING, __func__, "Data CRC match failed\n");
				data->track = SYNC_LAYER_CAN_TRANSMIT_FAILED;
			}
		}
	}

}

/******************RECEIVE*********************************/
/*
 * This should be called in thread or timer periodically
 * @param link 		: Link where data is going to be received
 * @param data		: Sync Layer Can Data
 * @param rxCallback: This is callback called if data received successfully or failed to receive data successfully
 * @return			: 1 rxCallback is called 0 for no callback called
 */
static uint8_t rxSendThread(SyncLayerCanLink *link,
		SyncLayerCanData *data,
		uint8_t (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
		void (*rxCallback)(SyncLayerCanLink *link, SyncLayerCanData *data,
				uint8_t status)) {
	uint8_t bytes[8] = { 0 };
	SyncLayerCanTrack prev_track = data->track;

	if (data->track == SYNC_LAYER_CAN_RECEIVE_SUCCESS) {
		rxCallback(link, data, 1);
		return 1;
	} else if (data->track == SYNC_LAYER_CAN_RECEIVE_FAILED) {
		rxCallback(link, data, 0);
		return 1;
	}

	/* Check */
	if (data->track == SYNC_LAYER_CAN_START_ACK) {
		/* START ACK */
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) ((uint32_t*) bytes + 1) = data->size;
		if (!canSend(link->start_ack_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_ERROR, __func__,
					"Start ack 0x%0x of data 0x%0x send failed\n",
					link->start_ack_ID, data->id);
		} else {
			/* Can sending success */
			data->track = SYNC_LAYER_CAN_DATA;
			console(CONSOLE_INFO, __func__,
					"Start request ACK 0x%0x of data 0x%0x send successful\n",
					link->start_ack_ID, data->id);
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA_ACK) {
		/* DATA ACK */
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) ((uint32_t*) bytes + 1) = data->count;
		if (!canSend(link->data_ack_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_ERROR, __func__,
					"Data received ack 0x%0x of data 0x%0x send failed\n",
					link->data_ack_ID, data->id);
		} else {
			/* Can sending success */
			console(CONSOLE_INFO, __func__,
					"Data received ack 0x%0x of data 0x%0x send successful\n",
					link->data_ack_ID, data->id);
			if (data->count == data->size) {
				/* Data receive complete */
				data->track = SYNC_LAYER_CAN_END_REQUEST;
			} else {
				data->track = SYNC_LAYER_CAN_DATA;
			}
		}
	} else if (data->track == SYNC_LAYER_CAN_DATA_COUNT_RESET_ACK) {
		/* DATA COUNT RESET ACK */
		*(uint32_t*) bytes = data->id;
		*(uint16_t*) ((uint32_t*) bytes + 1) = data->count;
		if (!canSend(link->data_count_reset_ack_ID, bytes, 8)) {
			/* Can sending success */
			console(CONSOLE_ERROR, __func__,
					"Data count reset request 0x%0x of data 0x%0x send failed\n",
					link->data_count_reset_ack_ID, data->id);
		} else {
			/* Can sending failed */
			console(CONSOLE_INFO, __func__,
					"Data count reset request 0x%0x of data 0x%0x send successful\n",
					link->data_count_reset_ack_ID, data->id);
			data->track = SYNC_LAYER_CAN_DATA;
		}
	} else if (data->track == SYNC_LAYER_CAN_END_ACK) {
		*(uint32_t*) bytes = data->id;
		uint32_t crc_from_sender = data->crc;
		data->crc = getCRCValue(data->bytes, data->size);
		if (data->crc == crc_from_sender)
			*(uint8_t*) ((uint32_t*) bytes + 1) = 0xFF;
		else
			*(uint8_t*) ((uint32_t*) bytes + 1) = 0x00;
		/* END ACK */
		if (!canSend(link->end_ack_ID, bytes, 8)) {
			/* Can sending failed */
			console(CONSOLE_ERROR, __func__,
					"Data received ack 0x%0x of data 0x%0x send failed\n",
					link->end_ack_ID, data->id);
		} else {
			/* Can sending success */
			data->track = data->crc == crc_from_sender?SYNC_LAYER_CAN_RECEIVE_SUCCESS:SYNC_LAYER_CAN_RECEIVE_FAILED;
			console(CONSOLE_INFO, __func__,
					"Data end ack 0x%0x of data 0x%0x send successful\n",
					link->end_ack_ID, data->id);
		}
	}

	if (prev_track != data->track) {
		data->time_elapse = timeInMillis();
		if (data->time_elapse == 0)
			console(CONSOLE_WARNING, __func__, "Time elapse %d\n",
					data->time_elapse);
	}

	/* Check transmit timeout */
	if ((timeInMillis() - data->time_elapse) > SYNC_LAYER_CAN_RECEIVE_TIMEOUT) {
		console(CONSOLE_WARNING, __func__, "Data receive 0x%0x timeout %d\n",
				data->id, SYNC_LAYER_CAN_RECEIVE_TIMEOUT);
		data->track = SYNC_LAYER_CAN_RECEIVE_FAILED;
	}

	return 0;
}

/*
 * This should be called in thread or timer periodically
 * @param link 			: Link where data is going to be received
 * @param data			: Sync Layer Can Data
 * @param can_id		: Can ID received
 * @param can_bytes		: Can bytes received
 * @param can_bytes_len	: Can bytes length
 */
static void rxReceiveThread(SyncLayerCanLink *link,
		SyncLayerCanData *data, uint32_t can_id, uint8_t *can_bytes,
		uint8_t can_bytes_len) {
	uint32_t data_id;
	uint8_t is_failed = 0;
	if (data->track == SYNC_LAYER_CAN_START_REQUEST
			&& can_id == link->start_req_ID) {
		/* START REQ */
		data_id = *(uint32_t*) can_bytes;
		uint16_t size = *(uint16_t*) ((uint32_t*) can_bytes + 1);
		console(CONSOLE_INFO, __func__,
				"Start ack 0x%0x of data 0x%0x success\n", link->start_req_ID,
				data->id);
		data->id = data_id;
		data->count = 0;
		data->size = size;
		data->track = SYNC_LAYER_CAN_START_ACK;
	} else if (data->track == SYNC_LAYER_CAN_DATA && can_id == data->id) {
		/* DATA */
		if (can_bytes_len < 8) {
			for (int i = 0; i < can_bytes_len; i++)
				data->bytes[data->count + i] = can_bytes[i];
		} else {
			*(uint64_t*) (&data->bytes[data->count]) = *(uint64_t*) can_bytes;
		}
		data->count += can_bytes_len;
		data->track = SYNC_LAYER_CAN_DATA_ACK;
		console(CONSOLE_INFO, __func__, "Data 0x%0x receive success\n",
				data->id);
	} else if (data->track == SYNC_LAYER_CAN_DATA
			&& can_id == link->data_count_reset_req_ID) {
		/* DATA COUNT RESET */
		data_id = *(uint32_t*) can_bytes;
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"Data count reset request 0x%0x of data 0x%0x contains wrong ID 0x%0x\n",
					link->data_count_reset_req_ID, data->id, data_id);
			is_failed = 1;
		} else {
			/* ID matched */
			console(CONSOLE_INFO, __func__,
					"Data count reset request 0x%0x of data 0x%0x receive success\n",
					link->data_count_reset_req_ID, data->id);
			uint16_t count = *(uint16_t*) ((uint32_t*) can_bytes + 1);
			data->track = SYNC_LAYER_CAN_DATA_COUNT_RESET_ACK;
			console(CONSOLE_WARNING, __func__,
					"Data 0x%0x count %d reset to %d\n", data_id, data->count,
					count);
			data->count = count;
		}
	} else if (data->track == SYNC_LAYER_CAN_END_REQUEST
			&& can_id == link->end_req_ID) {
		/* END */
		data_id = *(uint32_t*) can_bytes;
		data->crc = *((uint32_t*) can_bytes + 1);
		if (data_id != data->id) {
			/* ID doesn't match */
			console(CONSOLE_ERROR, __func__,
					"End ack 0x%0x of data 0x%0x contains wrong ID 0x%0x\n",
					link->end_req_ID, data->id, data_id);
			is_failed = 1;
		} else {
			/* ID matched */
			console(CONSOLE_INFO, __func__,
					"End ack 0x%0x of data 0x%0x receive success\n",
					link->end_req_ID, data->id);
			data->track = SYNC_LAYER_CAN_END_ACK;
		}
	}

	if (is_failed) {
		data->track = SYNC_LAYER_CAN_RECEIVE_FAILED;
	}
}


struct SyncLayerCanControl StaticSyncLayerCan = {
		.txSendThread = txSendThread,
		.txReceiveThread = txReceiveThread,
		.rxSendThread = rxSendThread,
		.rxReceiveThread = rxReceiveThread
};
