# PN CAN Protocol
This is protocol for transmitting data via CAN. Its features and pros are :  
→ Data of <b>any size</b> can be transmitted and received  
→ <b>Self recovery</b> of data in case there are missing data  
→ <b>CRC</b> check in both sides  
→ <b>Link</b> is provided virtual connection between two devices via which data can be transmitted and received  
→ Multiple links can be created  
→ It is higher layer and uses call back  

 It has its cons :  
→ <b>Link</b> can be connected only between two devices and hence only two device can share data two each others in one link

# Test Code for both sides :
In main.c
```rb

int main(){
	//Default Initialization...
	
	// Code before while loop
	extern void run();
  	run();
	while(1){}
}
```
In stm32f1xx_it.c
```rb
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
extern void canInterrupt();
/* USER CODE END PFP */

/**
 * @brief This function handles USB low priority or CAN RX0 interrupts.
 */
void USB_LP_CAN1_RX0_IRQHandler(void) {
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 0 */
	canInterrupt();
	/* USER CODE END USB_LP_CAN1_RX0_IRQn 0 */
	HAL_CAN_IRQHandler(&hcan);
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 1 */
	/* USER CODE END USB_LP_CAN1_RX0_IRQn 1 */
}

```
In pn_can_protocol_test.c
```rb
/*
 * pn_can_protocol_test.c
 *
 *  Created on: Nov 3, 2023
 *      Author: NIRUJA
 */
#include "main.h"
#include "pn_can_protocol.h"

#define TEST_ENABLE
//#define TX_ENABLE        //For side1 uncomment this line

#ifdef TEST_ENABLE
CANLink *canLink1;
CANLink *canLink2;

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
	StaticCANLink.canReceive(canLink1, rx_header.ExtId, bytes, rx_header.DLC);
	StaticCANLink.canReceive(canLink2, rx_header.ExtId, bytes, rx_header.DLC);
}

static CAN_TxHeaderTypeDef tx_header;
static uint32_t tx_mailbox;
static int canSend(uint32_t id, uint8_t *bytes, uint8_t len) {
	tx_header.DLC = len;
	tx_header.ExtId = id;
	tx_header.IDE = CAN_ID_EXT;
	tx_header.RTR = CAN_RTR_DATA;
	tx_header.TransmitGlobalTime = DISABLE;
	return HAL_CAN_AddTxMessage(&hcan, &tx_header, bytes, &tx_mailbox) == HAL_OK;
}

int txCallback(uint32_t id, uint8_t *bytes, uint16_t size, int status) {
	printf("TX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);
	if (!status) {
		printf("(failed)\n");
		return 1;
	}
	printf("\n");
	return 1;
}

int rxCallback(uint32_t id, uint8_t *bytes, uint16_t size, int status) {
	printf("RX|0x%0x>", (int) id);
	for (int i = 0; i < 8; i++)
		printf(" %d ", bytes[i]);
	if (!status) {
		printf("(failed)\n");
		return 1;
	}
	printf("\n");
	return 1;
}

uint8_t dataBytes1[8];
uint8_t dataBytes2[16];
uint8_t dataBytes3[32];
uint8_t buffer1[2048 + mapSize(2048, 8)];
uint8_t buffer2[2048 + mapSize(2048, 8)];
#ifdef TX_ENABLE
void run() {
	canInit();
	for (int i = 0; i < sizeof(dataBytes1); i++)
		dataBytes1[i] = i;
	for (int i = 0; i < sizeof(dataBytes2); i++)
		dataBytes2[i] = i;
	for (int i = 0; i < sizeof(dataBytes3); i++)
		dataBytes3[i] = i;

	BuddyHeap heap1 = StaticBuddyHeap.new(buffer1, sizeof(buffer1), 8);
	BuddyHeap heap2 = StaticBuddyHeap.new(buffer2, sizeof(buffer2), 8);
	canLink1 = StaticCANLink.new(0x1, 0x2, 0x3, 0x4, canSend, txCallback,
			rxCallback, &heap1, 0);
	canLink2 = StaticCANLink.new(0x11, 0x22, 0x33, 0x44, canSend, txCallback,
			rxCallback, &heap2, 0);

	HAL_Delay(3000);
	printf("----------------------TX INITIATING-----------------------\n");

	uint32_t prevMillis = HAL_GetTick();
	while (1) {
		if ((HAL_GetTick() - prevMillis) > 100) {
			StaticCANLink.addTxMsgPtr(canLink1, 0xA, dataBytes1,
					sizeof(dataBytes1));
			StaticCANLink.addTxMsgPtr(canLink1, 0xB, dataBytes2,
					sizeof(dataBytes2));
			StaticCANLink.addTxMsgPtr(canLink1, 0xC, dataBytes3,
					sizeof(dataBytes3));
			StaticCANLink.addTxMsgPtr(canLink2, 0xD, dataBytes1,
					sizeof(dataBytes1));
			StaticCANLink.addTxMsgPtr(canLink2, 0xE, dataBytes2,
					sizeof(dataBytes2));
			StaticCANLink.addTxMsgPtr(canLink2, 0xF, dataBytes3,
					sizeof(dataBytes3));
			prevMillis = HAL_GetTick();
		}
		StaticCANLink.thread(canLink1);
		StaticCANLink.thread(canLink2);
	}
}
#else
void run() {
	canInit();
	for (int i = 0; i < sizeof(dataBytes1); i++)
		dataBytes1[i] = i;
	for (int i = 0; i < sizeof(dataBytes2); i++)
		dataBytes2[i] = i;
	for (int i = 0; i < sizeof(dataBytes3); i++)
		dataBytes3[i] = i;

	BuddyHeap heap1 = StaticBuddyHeap.new(buffer1, sizeof(buffer1), 8);
	BuddyHeap heap2 = StaticBuddyHeap.new(buffer2, sizeof(buffer2), 8);
	canLink1 = StaticCANLink.new(0x1, 0x2, 0x3, 0x4, canSend, txCallback,
			rxCallback, &heap1,1);
	canLink2 = StaticCANLink.new(0x11, 0x22, 0x33, 0x44, canSend, txCallback,
			rxCallback, &heap2, 1);

	HAL_Delay(5000);
	printf("----------------------RX INITIATING-----------------------\n");

	uint32_t prevMillis = HAL_GetTick();
	while (1) {
		if ((HAL_GetTick() - prevMillis) > 100) {
			StaticCANLink.addTxMsgPtr(canLink1, 0xAA, dataBytes1,
					sizeof(dataBytes1));
			StaticCANLink.addTxMsgPtr(canLink1, 0xBB, dataBytes2,
					sizeof(dataBytes2));
			StaticCANLink.addTxMsgPtr(canLink1, 0xCC, dataBytes3,
					sizeof(dataBytes3));
			StaticCANLink.addTxMsgPtr(canLink2, 0xDD, dataBytes1,
					sizeof(dataBytes1));
			StaticCANLink.addTxMsgPtr(canLink2, 0xEE, dataBytes2,
					sizeof(dataBytes2));
			StaticCANLink.addTxMsgPtr(canLink2, 0xFF, dataBytes3,
					sizeof(dataBytes3));
			prevMillis = HAL_GetTick();
		}
		StaticCANLink.thread(canLink1);
		StaticCANLink.thread(canLink2);
	}
}
#endif

#endif
```


# Side 1 Output:
```rb
----------------------TX INITIATING-----------------------
RX|0xaa> 0  1  2  3  4  5  6  7 
RX|0xdd> 0  1  2  3  4  5  6  7 
RX|0xbb> 0  1  2  3  4  5  6  7 
RX|0xee> 0  1  2  3  4  5  6  7 
RX|0xcc> 0  1  2  3  4  5  6  7 
RX|0xff> 0  1  2  3  4  5  6  7 
TX|0xc> 0  1  2  3  4  5  6  7 
TX|0xa> 0  1  2  3  4  5  6  7 
TX|0xb> 0  1  2  3  4  5  6  7 
TX|0xd> 0  1  2  3  4  5  6  7 
TX|0xf> 0  1  2  3  4  5  6  7 
TX|0xe> 0  1  2  3  4  5  6  7 
RX|0xaa> 0  1  2  3  4  5  6  7 
RX|0xdd> 0  1  2  3  4  5  6  7 
RX|0xbb> 0  1  2  3  4  5  6  7 
RX|0xee> 0  1  2  3  4  5  6  7 
RX|0xcc> 0  1  2  3  4  5  6  7 
```
# Side 2 Output:
```rb
----------------------RX INITIATING-----------------------
TX|0xaa> 0  1  2  3  4  5  6  7 
TX|0xdd> 0  1  2  3  4  5  6  7 
TX|0xbb> 0  1  2  3  4  5  6  7 
TX|0xee> 0  1  2  3  4  5  6  7 
TX|0xcc> 0  1  2  3  4  5  6  7 
TX|0xff> 0  1  2  3  4  5  6  7 
RX|0xc> 0  1  2  3  4  5  6  7 
RX|0xa> 0  1  2  3  4  5  6  7 
RX|0xb> 0  1  2  3  4  5  6  7 
RX|0xd> 0  1  2  3  4  5  6  7 
RX|0xf> 0  1  2  3  4  5  6  7 
RX|0xe> 0  1  2  3  4  5  6  7 
TX|0xaa> 0  1  2  3  4  5  6  7 
TX|0xdd> 0  1  2  3  4  5  6  7 
TX|0xbb> 0  1  2  3  4  5  6  7 
TX|0xee> 0  1  2  3  4  5  6  7 
TX|0xcc> 0  1  2  3  4  5  6  7 
```
