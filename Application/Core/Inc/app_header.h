/*
 * app_header.h
 *
 *  Created on: Jul 6, 2026
 *      Author: muhammet
 */

#ifndef INC_APP_HEADER_H_
#define INC_APP_HEADER_H_

#include "flash_layout.h"

typedef struct
{
	uint32_t ota_flag;
	uint32_t magic;
	uint32_t size;
	uint32_t crc;
	uint32_t version;
} app_header_t;

#endif /* INC_APP_HEADER_H_ */
