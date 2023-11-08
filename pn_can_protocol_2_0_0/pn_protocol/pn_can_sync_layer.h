/*
 * pn_can_sync_layer.h
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */

#ifndef PN_CAN_SYNC_LAYER_H_
#define PN_CAN_SYNC_LAYER_H_
#include "stdint.h"

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
	int (*canSend)(uint32_t id, uint8_t *bytes, uint8_t len);

} SyncLayerCANLink;

struct SyncLayerCanControl{
	/**
	 * Send thread for transmitting should be called in thread continuously
	 * @param link 	: Link where data is to be transmitted
	 * @param data	: Data that is to be transmitted
	 */
	int (*txSendThread)(SyncLayerCANLink *link, SyncLayerCANData *data);

	/**
	 * Receive thread for transmitting should be called in thread where CAN data is being received
	 * @param link 	: Link where data is to be transmitted
	 * @param data	: Data that is to be transmitted
	 * @param id	: CAN ID
	 * @param bytes	: bytes received from CAN
	 * @param len	: length of CAN bytes received
	 */
	int (*txReceiveThread)(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
			uint8_t *bytes, uint16_t len);

	/**
	 * Receive thread for receiving should be called in thread continuously
	 * @param link 	: Link where data is to be received
	 * @param data	: Data that is to be received
	 */
	int (*rxSendThread)(SyncLayerCANLink *link, SyncLayerCANData *data);

	/**
	 * Receive thread for receiving should be called in thread where CAN data is being received
	 * @param link 	: Link where data is to be received
	 * @param data	: Data that is to be received
	 * @param id	: CAN ID
	 * @param bytes	: bytes received from CAN
	 * @param len	: length of CAN bytes received
	 */
	int (*rxReceiveThread)(SyncLayerCANLink *link, SyncLayerCANData *data, uint32_t id,
			uint8_t *bytes, uint16_t len);
};
extern struct SyncLayerCanControl StaticSyncLayerCan;


#endif /* PN_CAN_SYNC_LAYER_H_ */
