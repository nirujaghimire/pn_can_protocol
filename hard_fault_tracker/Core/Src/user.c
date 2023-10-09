/*
 * user.c
 *
 *  Created on: Aug 24, 2023
 *      Author: NIRUJA
 */

#include "main.h"
#include "malloc.h"


void print(){
	printf("I am ... \n");
//	*(uint32_t*) 0x20010000 = 0xFFFFFFFF;

//	int ar[0];
//	ar[2]=0;
	free(-1);

	int x = 2;
	int y = 2;
	int z = x+y;



}


void run(){

//	printf("Initiating....\n");

	/* enable handlers */
//	uint32_t *pSHCSR = (uint32_t*) 0xE000ED24;
//	*pSHCSR |= (1 << 16); // Memory Fault
//	*pSHCSR |= (1 << 17); // Bus Fault
//	*pSHCSR |= (1 << 18); // Usage Fault

	/* Enable Divide by Zero Trap */
//	uint32_t *pCCR = (uint32_t*) 0xE000ED14;
//	*pCCR |= (1 << 4); // Div by Zero

	///////////FAULT TESTING//////////////////
	/* Fill a meaningless value */
//	*(uint32_t*) 0x20010000 = 0xFFFFFFFF;

	/* Set PC with LSB being 1 to indicate Thumb State */
//	void (*pFunc)(void) = (void*)0x20010001;
//	void (*pFunc)(void) = print;

	/* call function */
//	pFunc();

//	print();



	int *x;
	while(1){
		x = malloc(1024);
		if(x==NULL)
			printf("NULL\n");
		else
			printf("OKAY\n");
		free(x);
		HAL_Delay(10);
	}
}

