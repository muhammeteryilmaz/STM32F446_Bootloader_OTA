/*
 * flash_layout.h
 *
 *  Created on: Jun 26, 2026
 *      Author: muhammet
 */

#ifndef INC_FLASH_LAYOUT_H_
#define INC_FLASH_LAYOUT_H_

#define BL_START_ADDR				0x08000000

#define APP_HEADER_ADDR				0x08008000

#define APP_START_ADDR				0x0800C000

#define APP_HEADER_SECTOR	FLASH_SECTOR_2

#define APP_START_SECTOR	FLASH_SECTOR_3

#define APP_END_SECTOR		FLASH_SECTOR_7

#define APP_MAX_SIZE		464*1024

#endif /* INC_FLASH_LAYOUT_H_ */
