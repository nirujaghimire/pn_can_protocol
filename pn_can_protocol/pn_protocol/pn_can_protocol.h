/*
 * pn_can_protocol.h
 *
 *  Created on: Jul 10, 2023
 *      Author: NIRUJA
 */

#ifndef PN_CAN_PROTOCOL_H_
#define PN_CAN_PROTOCOL_H_

#include "stdint.h"
#include "pn_can_sync_layer.h"

struct CanProtocolControl {
	/**
	 * This will add the link
	 * @param link			: Link where data is to be transmitted or received
	 * @param canSendFunc	: Function that sends can
	 * @param txCallbackFunc: Function that is called after data is transmitted successfully or failed to transmit successfully
	 * @param rxCallbackFunc: Function that is called after data is received successfully or failed to receive successfully
	 * @param is_que		: 1 for use of que in transmit side else uses map
	 * @return				: 1 if everything OK else 0
	 */
	uint8_t (*addLink)(SyncLayerCanLink *link,
			uint8_t (*canSendFunc)(uint32_t id, uint8_t *bytes, uint8_t len),
			uint8_t (*txCallbackFunc)(uint32_t id, uint8_t *bytes,
					uint16_t size, uint8_t status),
			uint8_t (*rxCallbackFunc)(uint32_t id, uint8_t *bytes,
					uint16_t size, uint8_t status), uint8_t is_que);

	/**
	 * This will pop the data in link
	 * @param link			: Link where data is being transmitted or received
	 * @return				: 1 for popped and 0 for fail to pop
	 */
	uint8_t (*pop)(SyncLayerCanLink *link);
	/*
	 * This will add the data to be transmitted in link
	 * @param link		: Link where data is to be transmitted
	 * @param id		: Can ID of message
	 * @param data		: Actual data is to be transmitted
	 * @param size		: Size of data
	 * @return			: 1 if successfully added else 0
	 */
	uint8_t (*addTxMessage)(SyncLayerCanLink *link, uint32_t id,
			uint8_t *data, uint16_t size);

	/*
	 * This will add reference of the data to be transmitted in link
	 * @param link		: Link where data is to be transmitted
	 * @param id		: Can ID of message
	 * @param data		: reference to the data is to be transmitted
	 * @param size		: Size of data
	 * @return			: 1 if successfully added else 0
	 */
	uint8_t (*addTxMessagePtr)(SyncLayerCanLink *link, uint32_t id,
			uint8_t *data, uint16_t size);
	/*
	 * This will add reference of the data as container to be received in link
	 * @param link		: Link where data is to be received
	 * @param id		: Can ID of message to be received
	 * @param data		: reference to the data is to be received
	 * @param size		: Size of data
	 * @return			: 1 if successfully added else 0
	 */
	uint8_t (*addRxMessagePtr)(SyncLayerCanLink *link, uint32_t id,
			uint8_t *data, uint16_t size);

	/*
	 * This should be called in thread or timer periodically
	 * @param link	: Link where data is to be transmitted or received
	 */
	void (*sendThread)(SyncLayerCanLink *link);

	/*
	 * This should be called in thread or timer periodically after data is received from CAN
	 * @param link	: Link where data is to be transmitted or received
	 */
	void (*recThread)(SyncLayerCanLink *link, uint32_t id,
			uint8_t *bytes, uint16_t len);
};



extern struct CanProtocolControl StaticCanProtocol;


struct CanProtocolTest{
	void (*canRxInterrupt)();
	void (*runRx)();
	void (*runTx)();
};
extern struct CanProtocolTest StaticCanProtocolTest;

#endif /* PN_CAN_PROTOCOL_H_ */
