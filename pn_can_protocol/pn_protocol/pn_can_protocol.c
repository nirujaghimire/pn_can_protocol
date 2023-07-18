/*
 * pn_can_protocol.c
 *
 *  Created on: Jul 10, 2023
 *      Author: NIRUJA
 */


#include "pn_can_protocol.h"
#include "stdarg.h"
#include "hash_map.h"
#include "queue.h"
#include "malloc.h"
#include "main.h"

#include "test.h"



#define PN_CAN_PROTOCOL_LINK_MAX_SIZE 10

static SyncLayerCanLink *links[PN_CAN_PROTOCOL_LINK_MAX_SIZE];
static uint8_t (*canSend[PN_CAN_PROTOCOL_LINK_MAX_SIZE])(uint32_t id,
		uint8_t *bytes, uint8_t len);
static uint8_t (*txCallback[PN_CAN_PROTOCOL_LINK_MAX_SIZE])(uint32_t id,
		uint8_t *bytes, uint16_t size, uint8_t status);
static uint8_t (*rxCallback[PN_CAN_PROTOCOL_LINK_MAX_SIZE])(uint32_t id,
		uint8_t *bytes, uint16_t size, uint8_t status);

static uint8_t link_size = 0;

static Queue *tx_que[PN_CAN_PROTOCOL_LINK_MAX_SIZE];
static uint8_t is_in_que[PN_CAN_PROTOCOL_LINK_MAX_SIZE] = { 0 };
static HashMap *tx_map[PN_CAN_PROTOCOL_LINK_MAX_SIZE];
static HashMap *rx_map[PN_CAN_PROTOCOL_LINK_MAX_SIZE];
static uint32_t memory_leak_tx[PN_CAN_PROTOCOL_LINK_MAX_SIZE];
static uint32_t memory_leak_rx[PN_CAN_PROTOCOL_LINK_MAX_SIZE];

/*****************************CONSOLE****************************/
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
		printf("pn_can_protocol.c|%s> ERROR :", func_name);
	} else if (status == CONSOLE_INFO) {
		GREEN;
		printf("pn_can_protocol.c|%s> INFO : ", func_name);
	} else if (status == CONSOLE_WARNING) {
		YELLOW;
		printf("pn_can_protocol.c|%s> WARNING : ", func_name);
	} else {
		printf("pn_can_protocol.c|%s: ", func_name);
	}
	va_list args;
	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
	RESET;
}

/***************************PRIVATE*****************************/
static int getLinkIndex(SyncLayerCanLink *link) {
	/* Get index of link if not found return -1 */
	for (int i = 0; i < PN_CAN_PROTOCOL_LINK_MAX_SIZE; i++) {
		if (links[i] == link)
			return i;
	}
	return -1;
}

static void txCallbackFunc(SyncLayerCanLink *link, SyncLayerCanData *data,
		uint8_t status) {
	int link_index = getLinkIndex(link);
	int is_cleared = txCallback[link_index](data->id, data->bytes, data->size,
			status);
	if (is_cleared) {
		if (is_in_que[link_index])
			StaticQueue.dequeue(tx_que[link_index]);
		else
			StaticHashMap.delete(tx_map[link_index], data->id);
		if (data->dynamically_alocated)
			free(data->bytes);
		free(data);
		memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
	}
}

static void rxCallbackFunc(SyncLayerCanLink *link, SyncLayerCanData *data,
		uint8_t status) {
	int link_index = getLinkIndex(link);
	int is_cleared = rxCallback[link_index](data->id, data->bytes, data->size,
			status);
	if (is_cleared) {
		StaticHashMap.delete(rx_map[link_index], data->id);
		if (data->dynamically_alocated)
			free(data->bytes);
		free(data);
		memory_leak_rx[link_index] -= sizeof(SyncLayerCanData);
	}
}

/**************************PUBLIC*******************************/

/**
 * This will add the link
 * @param link			: Link where data is to be transmitted or received
 * @param canSendFunc	: Function that sends can
 * @param txCallbackFunc: Function that is called after data is transmitted successfully or failed to transmit successfully
 * @param rxCallbackFunc: Function that is called after data is received successfully or failed to receive successfully
 * @param is_que		: 1 for use of que in transmit side else uses map
 * @return				: 1 if everything OK else 0
 */
static uint8_t addLink(SyncLayerCanLink *link,
		uint8_t (*canSendFunc)(uint32_t id, uint8_t *bytes, uint8_t len),
		uint8_t (*txCallbackFunc)(uint32_t id, uint8_t *bytes, uint16_t size,
				uint8_t status),
		uint8_t (*rxCallbackFunc)(uint32_t id, uint8_t *bytes, uint16_t size,
				uint8_t status), uint8_t is_que) {

	if (link == NULL) {
		console(CONSOLE_ERROR, __func__, "Link is NULL\n");
		return 0;
	}
	if (link_size >= PN_CAN_PROTOCOL_LINK_MAX_SIZE) {
		console(CONSOLE_ERROR, __func__, "Max size reached %d\n",
		PN_CAN_PROTOCOL_LINK_MAX_SIZE);
		return 0;
	}

	if (is_que) {
		tx_que[link_size] = StaticQueue.new(NULL);
		if (tx_que[link_size] == NULL) {
			console(CONSOLE_ERROR, __func__,
					"Heap is full. Tx Que can't be created\n");
			return 0;
		}
		is_in_que[link_size] = 1;
	} else {
		tx_map[link_size] = StaticHashMap.new();
		if (tx_map[link_size] == NULL) {
			console(CONSOLE_ERROR, __func__,
					"Heap is full. Tx Map can't be created\n");
			return 0;
		}
	}

	rx_map[link_size] = StaticHashMap.new();
	if (rx_map[link_size] == NULL) {
		console(CONSOLE_ERROR, __func__,
				"Heap is full. Rx Map can't be created\n");
		return 0;
	}

	links[link_size] = link;
	canSend[link_size] = canSendFunc;
	txCallback[link_size] = txCallbackFunc;
	rxCallback[link_size] = rxCallbackFunc;

	console(CONSOLE_INFO, __func__, "Link 0x%0x is added in index %d\n", link,
			link_size);

	link_size++;

	return 1;
}

/**
 * This will pop the data in link
 * @param link			: Link where data is being transmitted or received
 * @return				: 1 for popped and 0 for fail to pop
 */
static uint8_t pop(SyncLayerCanLink *link) {
	int link_index = getLinkIndex(link);
	if (link_index == -1) {
		console(CONSOLE_ERROR, __func__, "0x%0x link is not found.\n");
		return 0;
	}

	if (!is_in_que[link_index])
		return 0;
	SyncLayerCanData *data = (SyncLayerCanData*) StaticQueue.peek(tx_que[link_index]);
	if (data == NULL) {
		console(CONSOLE_WARNING, __func__, "Data is NULL\n");
		return 0;
	}
	console(CONSOLE_INFO, __func__, "Data of 0x%0x is found\n", data->id);
	StaticQueue.dequeue(tx_que[link_index]);
	if (data->dynamically_alocated)
		free(data->bytes);
	free(data);
	memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
	return 1;
}

/*
 * This will add the data to be transmitted in link
 * @param link		: Link where data is to be transmitted
 * @param id		: Can ID of message
 * @param data		: Actual data is to be transmitted
 * @param size		: Size of data
 * @return			: 1 if successfully added else 0
 */
static uint8_t addTxMessage(SyncLayerCanLink *link, uint32_t id,
		uint8_t *data, uint16_t size) {
	int link_index = getLinkIndex(link);
	if (link_index == -1) {
		console(CONSOLE_ERROR, __func__, "0x%0x link is not found.\n");
		return 0;
	}

	SyncLayerCanData *sync_data = (SyncLayerCanData*) malloc(
			sizeof(SyncLayerCanData));
	if (sync_data == NULL) {
		console(CONSOLE_ERROR, __func__,
				"Heap is full. Sync Data can't be created\n");
		return 0;
	}
	memory_leak_tx[link_index] += sizeof(SyncLayerCanData);

	uint8_t *data_bytes = (uint8_t*) malloc(size);
	if (data_bytes == NULL) {
		console(CONSOLE_ERROR, __func__,
				"Heap is full. data_bytes can't be allocated\n");
		return 0;
	}

	for (uint16_t i = 0; i < (size / 8); i++)
		*((uint64_t*) data_bytes + i) = *((uint64_t*) data + i);
	for (uint16_t i = (size / 8) * 8; i < size; i++)
		data_bytes[i] = data[i];


	sync_data->id = id;
	sync_data->bytes = data_bytes;
	sync_data->size = size;
	sync_data->track = SYNC_LAYER_CAN_START_REQUEST;
	sync_data->count = 0;
	sync_data->time_elapse = 0;
	sync_data->data_retry = 0;
	sync_data->dynamically_alocated = 1;

	if (is_in_que[link_index]) {
		if (StaticQueue.doesExist(tx_que[link_index], sync_data)) {
			console(CONSOLE_INFO, __func__, "Sync Data already in que\n");
			return 1;
		}
		if (StaticQueue.enqueue(tx_que[link_index], sync_data) == NULL) {
			console(CONSOLE_ERROR, __func__,
					"Heap is full. Sync data can't be put in que\n");
			return 0;
		}
	} else {
		if(!StaticHashMap.isKeyExist(tx_map[link_index], id)){
			if (StaticHashMap.insert(tx_map[link_index], id, sync_data) == NULL) {
						console(CONSOLE_ERROR, __func__,
						"Heap is full. Sync data can't be put in map\n");
				return 0;
			}
		}else{
			free(sync_data);
			memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
			console(CONSOLE_INFO, __func__,
									"Pointer of message with id 0x%0x is successfully updated\n", id);
			return 0;
		}
	}

	console(CONSOLE_INFO, __func__,
			"Message with id 0x%0x is successfully added\n", id);
	return 1;
}

/*
 * This will add reference of the data to be transmitted in link
 * @param link		: Link where data is to be transmitted
 * @param id		: Can ID of message
 * @param data		: reference to the data is to be transmitted
 * @param size		: Size of data
 * @return			: 1 if successfully added else 0
 */
static uint8_t addTxMessagePtr(SyncLayerCanLink *link, uint32_t id,
		uint8_t *data, uint16_t size) {
	int link_index = getLinkIndex(link);
	if (link_index == -1) {
		console(CONSOLE_ERROR, __func__, "0x%0x link is not found.\n");
		return 0;
	}

	SyncLayerCanData *sync_data = (SyncLayerCanData*) malloc(
			sizeof(SyncLayerCanData));
	if (sync_data == NULL) {
		console(CONSOLE_ERROR, __func__,
				"Heap is full. Sync Data can't be created\n");
		return 0;
	}
	memory_leak_tx[link_index] += sizeof(SyncLayerCanData);

	sync_data->id = id;
	sync_data->bytes = data;
	sync_data->size = size;
	sync_data->track = SYNC_LAYER_CAN_START_REQUEST;
	sync_data->count = 0;
	sync_data->time_elapse = 0;
	sync_data->data_retry = 0;
	sync_data->dynamically_alocated = 0;

	if (is_in_que[link_index]) {
		if (StaticQueue.doesExist(tx_que[link_index], sync_data)) {
			console(CONSOLE_INFO, __func__, "Sync Data already in que\n");
			return 1;
		}

		if (StaticQueue.enqueue(tx_que[link_index], sync_data) == NULL) {
			console(CONSOLE_ERROR, __func__,
					"Heap is full. Sync data can't be put in que\n");
			return 0;
		}
	} else {
		if(!StaticHashMap.isKeyExist(tx_map[link_index], id)){
			if (StaticHashMap.insert(tx_map[link_index], id, sync_data) == NULL) {
						console(CONSOLE_ERROR, __func__,
						"Heap is full. Sync data can't be put in map\n");
				return 0;
			}
		}else{
			free(sync_data);
			memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
			console(CONSOLE_INFO, __func__,
						"Pointer of message with id 0x%0x is successfully updated\n", id);
			return 0;
		}
	}
	console(CONSOLE_INFO, __func__,
			"Pointer of message with id 0x%0x is successfully added\n", id);
	return 1;
}

/*
 * This will add reference of the data as container to be received in link
 * @param link		: Link where data is to be received
 * @param id		: Can ID of message to be received
 * @param data		: reference to the data is to be received
 * @param size		: Size of data
 * @return			: 1 if successfully added else 0
 */
static uint8_t addRxMessagePtr(SyncLayerCanLink *link, uint32_t id,
		uint8_t *data, uint16_t size) {
	int link_index = getLinkIndex(link);
	if (link_index == -1) {
		console(CONSOLE_ERROR, __func__, "0x%0x link is not found.\n");
		return 0;
	}

	SyncLayerCanData *sync_data = (SyncLayerCanData*) malloc(
			sizeof(SyncLayerCanData));
	if (sync_data == NULL) {
		console(CONSOLE_ERROR, __func__,
				"Heap is full. Sync Data can't be created\n");
		return 0;
	}
	memory_leak_rx[link_index] += sizeof(SyncLayerCanData);

	sync_data->id = id;
	sync_data->bytes = data;
	sync_data->size = size;
	sync_data->track = SYNC_LAYER_CAN_START_REQUEST;
	sync_data->count = 0;
	sync_data->time_elapse = 0;
	sync_data->data_retry = 0;
	sync_data->dynamically_alocated = 0;

	if(!StaticHashMap.isKeyExist(rx_map[link_index], id)){
		if (StaticHashMap.insert(rx_map[link_index], id, sync_data) == NULL) {
			console(CONSOLE_ERROR, __func__,
						"Heap is full. Sync data can't be put\n");
			return 0;
		}
	}else{
		free(sync_data);
		memory_leak_rx[link_index] -= sizeof(SyncLayerCanData);
		console(CONSOLE_INFO, __func__,
								"Pointer of message with id 0x%0x is successfully updated\n", id);
		return 0;
	}
	console(CONSOLE_INFO, __func__,
			"Pointer of message with id 0x%0x is successfully added as container\n",
			id);
	return 1;
}

/*
 * This should be called in thread or timer periodically
 * @param link	: Link where data is to be transmitted or received
 */
static void sendThread(SyncLayerCanLink *link) {
	SyncLayerCanData *data;
	int link_index = getLinkIndex(link);

	if (is_in_que[link_index]) {
		/* Transmitting */
		data = (SyncLayerCanData*) StaticQueue.peek(tx_que[link_index]);
		if (data == NULL) {
			console(CONSOLE_WARNING, __func__, "Data is NULL\n");
			return;
		}
		console(CONSOLE_INFO, __func__, "Data of 0x%0x is found\n", data->id);
		StaticSyncLayerCan.txSendThread(link, data, canSend[link_index],
								txCallbackFunc);
	} else {
		/* Transmitting */
		int tx_keys_size = tx_map[link_index]->size;
		int tx_keys[tx_keys_size];
		StaticHashMap.getKeys(tx_map[link_index], tx_keys);
		for (int j = 0; j < tx_keys_size; j++) {
			data = (SyncLayerCanData*) StaticHashMap.get(tx_map[link_index], tx_keys[j]);
			if (data == NULL) {
				console(CONSOLE_WARNING, __func__,
						"Tx Key 0x%0x is not found\n", tx_keys[j]);
				continue;
			}
			console(CONSOLE_INFO, __func__, "Tx Key 0x%0x is found\n",
					tx_keys[j]);
			StaticSyncLayerCan.txSendThread(link, data, canSend[link_index],
					txCallbackFunc);
		}
	}

	/* Receiving */
	int rx_keys_size = rx_map[link_index]->size;
	int rx_keys[rx_keys_size];
	StaticHashMap.getKeys(rx_map[link_index], rx_keys);
	for (int j = 0; j < rx_keys_size; j++) {
		data = (SyncLayerCanData*) StaticHashMap.get(rx_map[link_index],
				rx_keys[j]);
		if (data == NULL) {
			console(CONSOLE_WARNING, __func__, "Rx Key 0x%0x is not found\n",
					rx_keys[j]);
			continue;
		}
		console(CONSOLE_INFO, __func__, "Rx Key 0x%0x is found\n", rx_keys[j]);
		StaticSyncLayerCan.rxSendThread(link, data, canSend[link_index],
				rxCallbackFunc);
	}
}

/*
 * This should be called in thread or timer periodically after data is received from CAN
 * @param link	: Link where data is to be transmitted or received
 */
static void recThread(SyncLayerCanLink *link, uint32_t id,
		uint8_t *bytes, uint16_t len) {
	SyncLayerCanData *data;
	int link_index = getLinkIndex(link);
	uint32_t data_id = id;

	/* Transmitting */
	uint8_t is_transmit = link->start_ack_ID == id || link->data_ack_ID == id
			|| link->data_count_reset_ack_ID == id || link->end_ack_ID == id;
	if (is_transmit) {
		if (is_in_que[link_index]) {
			data_id = *(uint32_t*) bytes;
			data = StaticQueue.peek(tx_que[link_index]);
			if (data == NULL) {
				console(CONSOLE_ERROR, __func__,
						"sync data doesn't exist in given tx map\n");
				return;
			}
		} else {
			data_id = *(uint32_t*) bytes;
			data = StaticHashMap.get(tx_map[link_index], data_id);
			if (data == NULL) {
				console(CONSOLE_ERROR, __func__,
						"sync data doesn't exist in given tx que\n");
				return;
			}
		}
		StaticSyncLayerCan.txReceiveThread(link, data, id, bytes, len);
		return;
	}

	/* Receiving */
	uint8_t is_receive = link->start_req_ID == id
			|| link->data_count_reset_req_ID == id || link->end_req_ID == id;
	if (is_receive) {
		//Is protocol id
		data_id = *(uint32_t*) bytes;
		data = StaticHashMap.get(rx_map[link_index], data_id);
		if (data == NULL && link->start_req_ID == id) {
			//Starting
			data = malloc(sizeof(SyncLayerCanData));
			if (data == NULL) {
				console(CONSOLE_ERROR, __func__,
						"Heap is full. Sync Data can't be created\n");
				return;
			}
			memory_leak_rx[link_index] += sizeof(SyncLayerCanData);
			uint16_t size = *(uint16_t*) ((uint32_t*) bytes + 1);

			data->id = data_id;
			data->bytes = (uint8_t*) malloc(size);
			data->size = size;
			data->track = SYNC_LAYER_CAN_START_REQUEST;
			data->count = 0;
			data->time_elapse = 0;
			data->data_retry = 0;
			data->dynamically_alocated = 1;

			if (StaticHashMap.insert(rx_map[link_index], data->id, data) == NULL) {
				console(CONSOLE_ERROR, __func__,
						"Heap is full. Sync data can't be put\n");
				return;
			}
		} else if (data == NULL) {
			console(CONSOLE_ERROR, __func__, "Data doesn't exist in rx_map\n");
			return;
		}
		StaticSyncLayerCan.rxReceiveThread(links[link_index], data, id, bytes, len);
		return;
	}
	data = StaticHashMap.get(rx_map[link_index], data_id);
	if (data == NULL) {
		console(CONSOLE_WARNING, __func__, "Data doesn't exist in rx_map\n");
		return;
	}
	StaticSyncLayerCan.rxReceiveThread(links[link_index], data, id, bytes, len);
	return;
}

struct CanProtocolControl StaticCanProtocol = {
		.addLink = addLink,
		.pop = pop,
		.addTxMessage = addTxMessage,
		.addTxMessagePtr = addTxMessagePtr,
		.addRxMessagePtr = addRxMessagePtr,
		.sendThread = sendThread,
		.recThread = recThread,
};


