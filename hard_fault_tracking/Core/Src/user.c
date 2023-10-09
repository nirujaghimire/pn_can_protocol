/*
 * user.c
 *
 *  Created on: Sep 27, 2023
 *      Author: peter
 */
#include "main.h"
#include "stdio.h"
#include "stdlib.h"

extern UART_HandleTypeDef huart1;

typedef struct {
	int real;
	int img;
} Complex;

void printComplex(Complex complex) {
	printf("(%d,%d)\n", complex.real, complex.img);
}

void printAddr(Complex *complex){
	printf("Complex : %p\n",complex);
	printf("->real : %p\n",&(complex->real));
	printf("->img : %p\n",&(complex->img));
}

int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t*) ptr, len, HAL_MAX_DELAY);
	return len;
}

Complex global = {3,4};


void run() {
	printf("Initiating.....\n");

	printf("\n");
	printComplex(global);
	printAddr(&global);

	printf("\n");
	static Complex comp = {1,2};
	printComplex(comp);
	printAddr(&comp);

	printf("\n");
	Complex *c = (Complex*)(0x8000010);
	c->real = 10;
	printComplex(*c);
	printAddr(c);
//	c->real = 26;
//	c->img = 14;

	printf("\n");
	Complex *complex = NULL;
	printComplex(*complex);
	printAddr(complex);

	printf("\n");
	Complex peter;
	printComplex(peter);
	printAddr(&peter);

	printf("\n");
	Complex *niruja = malloc(sizeof(Complex));
	printComplex(*niruja);
	printAddr(niruja);
	while (1) {

	}
}
