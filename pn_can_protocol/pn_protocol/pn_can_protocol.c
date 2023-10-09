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
#include "CAN_queue.h"

#include "test.h"

#define MAX_MEMORY 1024*15

#define MAX_LINK 2

typedef uint8_t (*CanSendFuncType)(uint32_t id, uint8_t *bytes, uint8_t len);
typedef uint8_t (*CallbackFuncType)(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status);

static SyncLayerCanLink *links[MAX_LINK];
static CanSendFuncType canSend[MAX_LINK];
static CallbackFuncType txCallback[MAX_LINK];
static CallbackFuncType rxCallback[MAX_LINK];

static uint8_t link_size = 0;
static CANQueue canTxQueue[MAX_LINK] = { 0 };
static CANQueue canRxQueue[MAX_LINK] = { 0 };
static Queue *tx_que[MAX_LINK];
static uint8_t is_in_que[MAX_LINK] = { 0 };
static HashMap *tx_map[MAX_LINK];
static HashMap *rx_map[MAX_LINK];
static int memory_leak_tx[MAX_LINK];
static int memory_leak_rx[MAX_LINK];
static BuddyHeap *heaps[MAX_LINK];

static int allocatedMemory = 0;

/*****************************CONSOLE****************************/
typedef enum {
	CONSOLE_ERROR, CONSOLE_INFO, CONSOLE_WARNING
} ConsoleStatus;
static int console(int condition, ConsoleStatus status, const char *func_name, const char *msg, ...) {
	if (!condition)
		return 0;
	return 1;

	if (status == CONSOLE_INFO)
		return 1;
	if (status == CONSOLE_WARNING)
		return 1;
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
	return 1;
}

/**
 * Check if memory pointer exist in defined memory
 */
static int validMemory(const char *func, BuddyHeap *heap, void *ptr) {
	if (ptr == NULL)
		return 1;
	uint32_t addr = (uint32_t) ptr;
	if (heap == NULL) {
		if (addr < 0x20000000 || addr > 0x20004fff) {
			printf("PC CAN-%s:\n", func);
			printf("Memory : 0x20000000 - 0x20004fff\n");
			printf("Ptr : %p\n\n", ptr);
//			*(uint8_t*)NULL = 10;
			return 0;
		}
	} else {
		if (!StaticBuddyHeap.isValidPointer(*heap, ptr)) {
			printf("HashMap-%s:\n", func);
			printf("Heap : %p\n", heap);
			printf("Memory : %p - %p\n", heap->memory, heap->memory + heap->maxSize);
			printf("Ptr : %p\n\n", ptr);
//			*(uint8_t*)NULL = 10;
			return 0;
		}

	}
	return 1;
}

/**
 * It allocates the memory and return pointer to it
 * @param heap : Pointer of static heap (OR) null for heap use
 * @param sizeInByte    : Size in bytes
 * @return              : Pointer to allocated memory
 *                      : NULL if there exist no memory for allocation
 */
static void* allocateMemory(BuddyHeap *heap, int sizeInByte) {
	void *ptr;
	ptr = heap != NULL ? StaticBuddyHeap.malloc(heap, sizeInByte) : malloc(sizeInByte);
	if (ptr != NULL)
		allocatedMemory += sizeInByte;
	return ptr;
}

/**
 * It free the allocated memory
 * @param heap : Pointer of static heap (OR) null for heap use
 * @param pointer       : Pointer to allocated Memory
 * @param sizeInByte    : Size to be freed
 * @return              : 1 for success (OR) 0 for failed
 */
static int freeMemory(BuddyHeap *heap, void *pointer, int sizeInByte) {
	heap != NULL ? StaticBuddyHeap.free(heap, pointer) : free(pointer);
	allocatedMemory -= sizeInByte;
	return 1;
}

/***************************PRIVATE*****************************/
static int getLinkIndex(SyncLayerCanLink *link) {
	/* Get index of link if not found return -1 */
	for (int i = 0; i < MAX_LINK; i++)
		if (links[i] == link)
			return i;
	return -1;
}

static void txCallbackFunc(SyncLayerCanLink *link, SyncLayerCanData *data, uint8_t status) {
	int link_index = getLinkIndex(link);
	int is_cleared = txCallback[link_index](data->id, data->bytes, data->size, status);
	if (is_cleared) {
		if (is_in_que[link_index])
			StaticQueue.dequeue(tx_que[link_index]);
		else
			StaticHashMap.delete(tx_map[link_index], data->id);
		if (data->dynamically_alocated)
			freeMemory(heaps[link_index], data->bytes, data->size);
		freeMemory(heaps[link_index], data, sizeof(SyncLayerCanData));
		memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
	}
}

static void rxCallbackFunc(SyncLayerCanLink *link, SyncLayerCanData *data, uint8_t status) {
	int link_index = getLinkIndex(link);
	int is_cleared = rxCallback[link_index](data->id, data->bytes, data->size, status);
	if (is_cleared) {
		StaticHashMap.delete(rx_map[link_index], data->id);
		if (data->dynamically_alocated)
			freeMemory(heaps[link_index], data->bytes, data->size);
		freeMemory(heaps[link_index], data, sizeof(SyncLayerCanData));
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
static uint8_t addLink(BuddyHeap *heap, SyncLayerCanLink *link, uint8_t (*canSendFunc)(uint32_t id, uint8_t *bytes, uint8_t len), uint8_t (*txCallbackFunc)(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status), uint8_t (*rxCallbackFunc)(uint32_t id, uint8_t *bytes, uint16_t size, uint8_t status), uint8_t is_que) {
	if (console(link == NULL, CONSOLE_ERROR, __func__, "Link is NULL\n"))
		return 0;
	if (console(link_size >= MAX_LINK, CONSOLE_ERROR, __func__, "Max size reached %d\n", MAX_LINK))
		return 0;
	canTxQueue[link_size] = StaticCANQueue.new();
	canRxQueue[link_size] = StaticCANQueue.new();

	if (is_que) {
		tx_que[link_size] = StaticQueue.new(heap, NULL);
		if (console(tx_que[link_size] == NULL, CONSOLE_ERROR, __func__, "Heap is full. Tx Que can't be created\n"))
			return 0;
		is_in_que[link_size] = 1;
	} else {
		tx_map[link_size] = StaticHashMap.new(heap);
		if (console(tx_map[link_size] == NULL, CONSOLE_ERROR, __func__, "Heap is full. Tx Map can't be created\n"))
			return 0;
	}

	rx_map[link_size] = StaticHashMap.new(heap);
	if (console(rx_map[link_size] == NULL, CONSOLE_ERROR, __func__, "Heap is full. Rx Map can't be created\n"))
		return 0;

	links[link_size] = link;
	canSend[link_size] = canSendFunc;
	txCallback[link_size] = txCallbackFunc;
	rxCallback[link_size] = rxCallbackFunc;

	console(1, CONSOLE_INFO, __func__, "Link 0x%0x is added in index %d\n", link, link_size);

	heaps[link_size] = heap;
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
	if (console(link_index == -1, CONSOLE_ERROR, __func__, "0x%0x link is not found.\n"))
		return 0;

	if (!is_in_que[link_index])
		return 0;
	SyncLayerCanData *data = (SyncLayerCanData*) StaticQueue.peek(tx_que[link_index]);
	if (console(data == NULL, CONSOLE_WARNING, __func__, "Data is NULL\n"))
		return 0;

	console(1, CONSOLE_INFO, __func__, "Data of 0x%0x is found\n", data->id);
	StaticQueue.dequeue(tx_que[link_index]);
	if (data->dynamically_alocated)
		freeMemory(heaps[link_index], data->bytes, data->size);
	freeMemory(heaps[link_index], data, sizeof(SyncLayerCanData));
	memory_leak_tx[link_index] -= sizeof(SyncLayerCanData);
	return 1;
}

static int doesExist(Queue *queue, SyncLayerCanData *data) {
	int que_size = queue->size;
	struct QueueData *queData = queue->front;
	SyncLayerCanData *syncData;
	for (int i = 0; i < que_size; i++) {
		syncData = (SyncLayerCanData*) queData->value;
		if (syncData->id == data->id)
			return 1;
		queData = queData->next;
		if (queData == NULL)
			break;
	}
	return 0;
}

static int doesIDExistInQueue(Queue *queue, uint32_t ID) {
	int que_size = queue->size;
	struct QueueData *queData = queue->front;
	SyncLayerCanData *syncData;
	for (int i = 0; i < que_size; i++) {
		syncData = (SyncLayerCanData*) queData->value;
		if (syncData->id == ID)
			return 1;
		queData = queData->next;
		if (queData == NULL)
			break;
	}
	return 0;
}

static int doesIDExistInMap(HashMap *map, uint32_t ID) {
	return StaticHashMap.isKeyExist(map, ID);
}

static int isLinkID(int linkIndex, uint32_t ID) {
	SyncLayerCanLink *link = links[linkIndex];
	return link->start_ack_ID == ID || link->data_count_reset_ack_ID == ID || link->data_ack_ID == ID || link->end_ack_ID == ID || link->start_req_ID == ID || link->data_count_reset_req_ID == ID || link->end_req_ID == ID;
}

/*
 * This will add the data to be transmitted in link
 * @param link		: Link where data is to be transmitted
 * @param id		: Can ID of message
 * @param data		: Actual data is to be transmitted
 * @param size		: Size of data
 * @return			: 1 if successfully added else 0
 */
static uint8_t addTxMessage(SyncLayerCanLink *link, uint32_t id, uint8_t *data, uint16_t size) {
	int index = getLinkIndex(link);
	if (console(index == -1, CONSOLE_ERROR, __func__, "0x%0x link is not found.\n"))
		return 0;
	SyncLayerCanData *sync_data = (SyncLayerCanData*) allocateMemory(heaps[index], sizeof(SyncLayerCanData));
	if (!validMemory(__func__, heaps[index], sync_data))
		return 0;
	if (console(sync_data == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync Data can't be created\n"))
		return 0;
	memory_leak_tx[index] += sizeof(SyncLayerCanData);

	uint8_t *data_bytes = (uint8_t*) allocateMemory(heaps[index], size);
	if (!validMemory(__func__, heaps[index], data_bytes))
		return 0;
	if (console(data_bytes == NULL, CONSOLE_ERROR, __func__, "Heap is full. data_bytes can't be allocated\n"))
		return 0;

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

	if (is_in_que[index]) {
		if (!doesExist(tx_que[index], sync_data)) {
			if (console(StaticQueue.enqueue(tx_que[index], sync_data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put in que\n"))
				return 0;
		} else {
			freeMemory(heaps[index], data_bytes, sync_data->size);
			freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
			memory_leak_tx[index] -= sizeof(SyncLayerCanData);
			console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is already exist updated\n", id);
			return 0;
		}
	} else {
		if (!StaticHashMap.isKeyExist(tx_map[index], id)) {
			if (console(StaticHashMap.insert(tx_map[index], id, sync_data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put in map\n"))
				return 0;
		} else {
			freeMemory(heaps[index], data_bytes, sync_data->size);
			freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
			memory_leak_tx[index] -= sizeof(SyncLayerCanData);
			console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is successfully updated\n", id);
			return 0;
		}
	}

	console(1, CONSOLE_INFO, __func__, "Message with id 0x%0x is successfully added\n", id);
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
static uint8_t addTxMessagePtr(SyncLayerCanLink *link, uint32_t id, uint8_t *data, uint16_t size) {
	int index = getLinkIndex(link);
	if (console(index == -1, CONSOLE_ERROR, __func__, "link is not found.\n"))
		return 0;

	SyncLayerCanData *sync_data = (SyncLayerCanData*) allocateMemory(heaps[index], sizeof(SyncLayerCanData));
	if (!validMemory(__func__, heaps[index], sync_data))
		return 0;
	if (console(sync_data == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync Data can't be created\n"))
		return 0;
	memory_leak_tx[index] += sizeof(SyncLayerCanData);

	sync_data->id = id;
	sync_data->bytes = data;
	sync_data->size = size;
	sync_data->track = SYNC_LAYER_CAN_START_REQUEST;
	sync_data->count = 0;
	sync_data->time_elapse = 0;
	sync_data->data_retry = 0;
	sync_data->dynamically_alocated = 0;

	if (is_in_que[index]) {
		if (!doesExist(tx_que[index], sync_data)) {
			if (console(StaticQueue.enqueue(tx_que[index], sync_data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put in que\n"))
				return 0;
		} else {
			freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
			memory_leak_tx[index] -= sizeof(SyncLayerCanData);
			console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is already exist updated\n", id);
			return 0;
		}
	} else {
		if (!StaticHashMap.isKeyExist(tx_map[index], id)) {
			if (console(StaticHashMap.insert(tx_map[index], id, sync_data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put in map\n")) {
				freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
				memory_leak_tx[index] -= sizeof(SyncLayerCanData);
				return 0;
			}
		} else {
			freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
			memory_leak_tx[index] -= sizeof(SyncLayerCanData);
			console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is successfully updated\n", id);
			return 0;
		}
	}
	console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is successfully added\n", id);
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
static uint8_t addRxMessagePtr(SyncLayerCanLink *link, uint32_t id, uint8_t *data, uint16_t size) {
	int index = getLinkIndex(link);
	if (console(index == -1, CONSOLE_ERROR, __func__, "0x%0x link is not found.\n"))
		return 0;

	SyncLayerCanData *sync_data = (SyncLayerCanData*) allocateMemory(heaps[index], sizeof(SyncLayerCanData));
	if (!validMemory(__func__, heaps[index], sync_data))
		return 0;
	if (console(sync_data == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync Data can't be created\n"))
		return 0;
	memory_leak_rx[index] += sizeof(SyncLayerCanData);

	sync_data->id = id;
	sync_data->bytes = data;
	sync_data->size = size;
	sync_data->track = SYNC_LAYER_CAN_START_REQUEST;
	sync_data->count = 0;
	sync_data->time_elapse = 0;
	sync_data->data_retry = 0;
	sync_data->dynamically_alocated = 0;

	if (!StaticHashMap.isKeyExist(rx_map[index], id)) {
		if (console(StaticHashMap.insert(rx_map[index], id, sync_data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put\n"))
			return 0;
	} else {
		freeMemory(heaps[index], sync_data, sizeof(SyncLayerCanData));
		memory_leak_rx[index] -= sizeof(SyncLayerCanData);
		console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is successfully updated\n", id);
		return 0;
	}
	console(1, CONSOLE_INFO, __func__, "Pointer of message with id 0x%0x is successfully added as container\n", id);
	return 1;
}

static void sendThread(SyncLayerCanLink *link) {
	SyncLayerCanData *data;
	int index = getLinkIndex(link);

	if (memory_leak_tx[index] <= MAX_MEMORY) {
		if (is_in_que[index]) {
			/* Transmitting */
			data = (SyncLayerCanData*) StaticQueue.peek(tx_que[index]);
			if (validMemory(__func__, heaps[index], data)) {
				if (data == NULL) {
					console(1, CONSOLE_WARNING, __func__, "Data is NULL\n");
				} else {
					console(1, CONSOLE_INFO, __func__, "Data of 0x%0x is found\n", data->id);
					StaticSyncLayerCan.txSendThread(link, data, canSend[index], txCallbackFunc);
				}
			}
		} else {
			/* Transmitting */
			int tx_keys_size = tx_map[index]->size;
			int tx_keys[tx_keys_size];

			int len = 0;
			if (StaticHashMap.getKeys(tx_map[index], tx_keys, &len) != NULL) {
				if (len > tx_keys_size)
					len = tx_keys_size;
				for (int j = 0; j < len; j++) {
					data = (SyncLayerCanData*) StaticHashMap.get(tx_map[index], tx_keys[j]);
					if (!validMemory(__func__, heaps[index], data))
						continue;
					if (data == NULL) {
						console(1, CONSOLE_WARNING, __func__, "Tx Key 0x%0x is not found\n", tx_keys[j]);
						continue;
					}
					console(1, CONSOLE_INFO, __func__, "Tx Key 0x%0x is found\n", tx_keys[j]);
					StaticSyncLayerCan.txSendThread(link, data, canSend[index], txCallbackFunc);
				}
			}

		}
	} else {
		console(1, CONSOLE_ERROR, __func__, "Tx Link %d Memory leak %d exceeds limit %d\n", index, memory_leak_tx[index], MAX_MEMORY);
	}

	/* Receiving */
	if (memory_leak_rx[index] <= MAX_MEMORY) {
		int rx_keys_size = rx_map[index]->size;
		int rx_keys[rx_keys_size];
		int len = 0;
		StaticHashMap.getKeys(rx_map[index], rx_keys, &len);
		if (StaticHashMap.getKeys(rx_map[index], rx_keys, &len) != NULL) {
			if (len > rx_keys_size)
				len = rx_keys_size;
			for (int j = 0; j < len; j++) {
				data = (SyncLayerCanData*) StaticHashMap.get(rx_map[index], rx_keys[j]);
				if (!validMemory(__func__, heaps[index], data))
					continue;
				if (data == NULL) {
					console(1, CONSOLE_WARNING, __func__, "Rx Key 0x%0x is not found\n", rx_keys[j]);
					continue;
				}
				console(1, CONSOLE_INFO, __func__, "Rx Key 0x%0x is found\n", rx_keys[j]);
				StaticSyncLayerCan.rxSendThread(link, data, canSend[index], rxCallbackFunc);
			}
		}

	} else {
		console(1, CONSOLE_ERROR, __func__, "Rx Link %d Memory leak %d exceeds limit %d\n", index, memory_leak_rx[index],
		MAX_MEMORY);
	}
}

static void recThread(SyncLayerCanLink *link, uint32_t id, uint8_t *bytes, uint16_t len) {
	SyncLayerCanData *data;
	int index = getLinkIndex(link);
	if (console(index < 0, CONSOLE_ERROR, __func__, "link index is negative : %d\n", index))
		return;

	uint32_t data_id = id;

	/* Transmitting */
	uint8_t is_transmit = (link->start_ack_ID == id) || (link->data_ack_ID == id) || (link->data_count_reset_ack_ID == id) || (link->end_ack_ID == id);
	if (is_transmit) {
		if (is_in_que[index]) {
			data_id = *(uint32_t*) bytes;
			data = StaticQueue.peek(tx_que[index]);
			if (console(data == NULL, CONSOLE_ERROR, __func__, "sync data doesn't exist in given tx map\n"))
				return;
		} else {
			data_id = *(uint32_t*) bytes;
			data = StaticHashMap.get(tx_map[index], data_id);
			if (!validMemory(__func__, heaps[index], data))
				return;
			if (console(data == NULL, CONSOLE_ERROR, __func__, "sync data doesn't exist in given tx que\n"))
				return;
		}
		StaticSyncLayerCan.txReceiveThread(link, data, id, bytes, len);
		return;
	}

	/* Receiving */
	uint8_t is_receive = (link->start_req_ID == id) || (link->data_count_reset_req_ID == id) || (link->end_req_ID == id);
	if (is_receive) {
		//Is protocol id
		data_id = *(uint32_t*) bytes;
		data = StaticHashMap.get(rx_map[index], data_id);
		if (data == NULL && link->start_req_ID == id) {
			//Starting
			data = allocateMemory(heaps[index], sizeof(SyncLayerCanData));
			if (!validMemory(__func__, heaps[index], data))
				return;
			if (console(data == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync Data can't be created\n"))
				return;
			memory_leak_rx[index] += sizeof(SyncLayerCanData);
			uint16_t size = *(uint16_t*) ((uint32_t*) bytes + 1);

			data->id = data_id;
			data->bytes = (uint8_t*) allocateMemory(heaps[index], size);
			data->size = size;
			data->track = SYNC_LAYER_CAN_START_REQUEST;
			data->count = 0;
			data->time_elapse = 0;
			data->data_retry = 0;
			data->dynamically_alocated = 1;

			if (console(StaticHashMap.insert(rx_map[index], data->id, data) == NULL, CONSOLE_ERROR, __func__, "Heap is full. Sync data can't be put\n"))
				return;
		} else if (console(data == NULL, CONSOLE_ERROR, __func__, "Data doesn't exist in rx_map\n")) {
			return;
		}
		StaticSyncLayerCan.rxReceiveThread(links[index], data, id, bytes, len);
		return;
	}
	data = StaticHashMap.get(rx_map[index], data_id);
	if (!validMemory(__func__, heaps[index], data))
		return;
	if (console(data == NULL, CONSOLE_INFO, __func__, "Data doesn't exist in rx_map\n")) {
		return;
	}
	StaticSyncLayerCan.rxReceiveThread(links[index], data, id, bytes, len);
}

/*
 * This should be called in thread or timer periodically
 * @param link	: Link where data is to be transmitted or received
 */
static void thread(SyncLayerCanLink *link) {
	int index = getLinkIndex(link);
	if (console(index == -1, CONSOLE_ERROR, __func__, "0x%0x link is not found.\n"))
		return;
	sendThread(link);

	CANData data = StaticCANQueue.dequeue(&canRxQueue[index]);
	if (data.ID != -1)
		recThread(link, data.ID, data.byte, 8);
}

/*
 * This should be called after CAN data is received from CAN
 * @param link	: Link where data is to be transmitted or received
 */
static void recCAN(SyncLayerCanLink *link, uint32_t ID, uint8_t *bytes) {
	int index = getLinkIndex(link);
	if (console(index == -1, CONSOLE_ERROR, __func__, "0x%0x link is not found.\n"))
		return;

	if (isLinkID(index, ID)) {
		StaticCANQueue.enqueue(&canRxQueue[index], ID, bytes);
	} else {
		if (!doesIDExistInMap(rx_map[index], ID))
			return;
		StaticCANQueue.enqueue(&canRxQueue[index], ID, bytes);
	}
}

static int getAllocatedMemories() {
	return allocatedMemory;
}

void printQueue() {
//	StaticCANQueue.print(&canTxQueue[0]);
//	StaticCANQueue.print(&canTxQueue[1]);
	StaticCANQueue.print(&canRxQueue[0]);
	StaticCANQueue.print(&canRxQueue[1]);
}

struct CanProtocolControl StaticCanProtocol = { .addLink = addLink, .pop = pop, .addTxMessage = addTxMessage, .addTxMessagePtr = addTxMessagePtr, .addRxMessagePtr = addRxMessagePtr, .thread = thread, .recCAN = recCAN, .getAllocatedMemories = getAllocatedMemories };

