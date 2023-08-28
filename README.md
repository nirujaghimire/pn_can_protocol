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

# Side 1:
In main.c
```rb

int main(){
	//Default Initialization...
	
	// Code before while loop
	#include "pn_can_protocol.h"
	 StaticCanProtocolTest.runRx();
	while(1){}
}
```
In stm32f1xx_it.c
```rb
/**
 * @brief This function handles USB low priority or CAN RX0 interrupts.
 */
void USB_LP_CAN1_RX0_IRQHandler(void) {
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 0 */
#include "pn_can_protocol.h"
	StaticCanProtocolTest.canRxInterrupt();
	/* USER CODE END USB_LP_CAN1_RX0_IRQn 0 */
	HAL_CAN_IRQHandler(&hcan);
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 1 */

	/* USER CODE END USB_LP_CAN1_RX0_IRQn 1 */
}

```

# Side 2:

In main.c
```rb

int main(){
	//Default Initialization...
	
	// Code before while loop
	#include "pn_can_protocol.h"
	 StaticCanProtocolTest.runTx();
	while(1){}
}
```
In stm32f1xx_it.c
```rb
/**
 * @brief This function handles USB low priority or CAN RX0 interrupts.
 */
void USB_LP_CAN1_RX0_IRQHandler(void) {
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 0 */
#include "pn_can_protocol.h"
	StaticCanProtocolTest.canTxInterrupt();
	/* USER CODE END USB_LP_CAN1_RX0_IRQn 0 */
	HAL_CAN_IRQHandler(&hcan);
	/* USER CODE BEGIN USB_LP_CAN1_RX0_IRQn 1 */

	/* USER CODE END USB_LP_CAN1_RX0_IRQn 1 */
}
```

# Side 1 Output:
```rb
SOURCE INIT:: SUCCESS
Tx Data1 : 0xb1 -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xa1 -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Tx Data1 : 0xc1 -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xa -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Rx Data1 : 0xc -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xb -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data2 : 0xd -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data2 : 0xe -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xb1 -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xa1 -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Tx Data1 : 0xc1 -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xa -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Rx Data1 : 0xc -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xb -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data2 : 0xd -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data2 : 0xe -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0
```
# Side 2 Output:
```rb
SOURCE INIT:: SUCCESS
Rx Data1 : 0xb1 -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xa1 -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Rx Data1 : 0xc1 -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xa -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Tx Data1 : 0xc -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xb -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xd -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xe -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xb1 -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xa1 -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Rx Data1 : 0xc1 -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xa -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Tx Data1 : 0xc -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xb -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xd -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xe -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xb1 -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Rx Data1 : 0xa1 -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Rx Data1 : 0xc1 -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xa -> 1 2 3 4 0 0 0 0 0 10 0 0 0 0 0 0 
Tx Data1 : 0xc -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data1 : 0xb -> 1 5 3 7 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xd -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0 
Tx Data2 : 0xe -> 2 4 6 8 0 0 0 0 0 0 0 0 0 0 0 0
```
