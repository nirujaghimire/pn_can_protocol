/*
 * pn_can_sync_layer.h
 *
 *  Created on: Jul 7, 2023
 *      Author: NIRUJA
 */

#ifndef PN_CAN_SYNC_LAYER_H_
#define PN_CAN_SYNC_LAYER_H_


#include "stdint.h"

typedef enum {
	SYNC_LAYER_CAN_START_REQUEST,
	SYNC_LAYER_CAN_START_ACK,
	SYNC_LAYER_CAN_DATA,
	SYNC_LAYER_CAN_DATA_COUNT_RESET_REQUEST,
	SYNC_LAYER_CAN_DATA_COUNT_RESET_ACK,
	SYNC_LAYER_CAN_DATA_ACK,
	SYNC_LAYER_CAN_END_REQUEST,
	SYNC_LAYER_CAN_END_ACK,
	SYNC_LAYER_CAN_TRANSMIT_SUCCESS,
	SYNC_LAYER_CAN_RECEIVE_SUCCESS,
	SYNC_LAYER_CAN_TRANSMIT_FAILED,
	SYNC_LAYER_CAN_RECEIVE_FAILED
} SyncLayerCanTrack;

typedef struct {
	uint32_t start_req_ID;
	uint32_t start_ack_ID;
	uint32_t data_count_reset_req_ID;
	uint32_t data_count_reset_ack_ID;
	uint32_t data_ack_ID;
	uint32_t end_req_ID;
	uint32_t end_ack_ID;
} SyncLayerCanLink;

typedef struct {
	uint32_t id;
	uint8_t *bytes;
	uint16_t size;
	SyncLayerCanTrack track;		// Initially should SYNC_LAYER_CAN_START_REQUEST
	uint8_t oneShot;        		// 1 for one data ack, 0 for each time data ack
	uint16_t count;         		// Initialization not required
	uint32_t time_elapse;   		// Initialization not required
	uint8_t data_retry;				// Initially should be 0
	uint8_t dynamically_alocated;	// 1 mean have to free bytes
	uint32_t crc;					// Not needed to load
} SyncLayerCanData;

struct SyncLayerCanControl{
	/******************TRANSMIT***************************/
	/*
	 * This should be called in thread or timer periodically
	 * @param link 		: Link where data is going to be transmitted
	 * @param data		: Sync Layer Can Data
	 * @param txCallback: This is callback called if data transmitted successfully or failed to transmit data
	 * @return			: 1 txCallback is called 0 for no callback called
	 */
	uint8_t (*txSendThread)(SyncLayerCanLink *link,
			SyncLayerCanData *data,
			uint8_t (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
			void (*txCallback)(SyncLayerCanLink *link, SyncLayerCanData *data,
					uint8_t status));

	/*
	 * This should be called in thread or timer periodically
	 * @param link 			: Link where data is going to be transmitted
	 * @param data			: Sync Layer Can Data
	 * @param can_id		: Can ID received
	 * @param can_bytes		: Can bytes received
	 * @param can_bytes_len	: Can bytes length
	 */
	void (*txReceiveThread)(SyncLayerCanLink *link,
			SyncLayerCanData *data, uint32_t can_id, uint8_t *can_bytes,
			uint8_t can_bytes_len);


	/******************RECEIVE*********************************/
	/*
	 * This should be called in thread or timer periodically
	 * @param link 		: Link where data is going to be received
	 * @param data		: Sync Layer Can Data
	 * @param rxCallback: This is callback called if data received successfully or failed to receive data successfully
	 * @return			: 1 rxCallback is called 0 for no callback called
	 */
	uint8_t (*rxSendThread)(SyncLayerCanLink *link,
			SyncLayerCanData *data,
			uint8_t (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len),
			void (*rxCallback)(SyncLayerCanLink *link, SyncLayerCanData *data,
					uint8_t status));

	/*
	 * This should be called in thread or timer periodically
	 * @param link 			: Link where data is going to be received
	 * @param data			: Sync Layer Can Data
	 * @param can_id		: Can ID received
	 * @param can_bytes		: Can bytes received
	 * @param can_bytes_len	: Can bytes length
	 */
	void (*rxReceiveThread)(SyncLayerCanLink *link,
			SyncLayerCanData *data, uint32_t can_id, uint8_t *can_bytes,
			uint8_t can_bytes_len);

};
extern struct SyncLayerCanControl StaticSyncLayerCan;

struct SyncLayerCanTest{
	void (*canRxInterruptRx)();
	void (*canRxInterruptTx)();
	void (*runRx)();
	void (*runTx)();
};
extern struct SyncLayerCanTest StaticSyncLayerCanTest;

#endif /* PN_CAN_SYNC_LAYER_H_ */
