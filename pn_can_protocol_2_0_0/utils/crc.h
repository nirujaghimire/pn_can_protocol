/*
 * crc.h
 *
 *  Created on: May 9, 2023
 *      Author: peter
 */

#ifndef CRC_H_
#define CRC_H_
#include "stdint.h"

void crc_init(void);
uint32_t crc32_accumulate(uint8_t *bytes,uint32_t len);
uint32_t crc32_calculate(uint8_t *bytes,uint32_t len);

#endif /* CRC_H_ */
