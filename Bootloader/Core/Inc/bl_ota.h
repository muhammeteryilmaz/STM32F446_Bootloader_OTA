/*
 * bl_ota.h
 *
 *  Created on: Jul 16, 2026
 *      Author: muhammet
 */

#ifndef INC_BL_OTA_H_
#define INC_BL_OTA_H_

int check_ota_request(void);
void clear_ota_flag(void);


typedef struct
{
	uint32_t write_addr;
	uint32_t total_received;
	uint32_t image_size;
	uint32_t expected_crc;
	uint32_t running_crc;
} bl_ota_ctx_t;

typedef struct
{
	uint32_t magic;
	uint32_t image_size;
	uint32_t crc;
	uint32_t version;
} ota_image_hdr_t;


typedef struct
{
	int (*read)(uint8_t *buf, uint32_t len);
	void(*set_total_size)(uint32_t total_size);
} ota_stream_t;

int bl_ota_run(bl_ota_ctx_t *ctx, ota_stream_t *stream);

#endif /* INC_BL_OTA_H_ */
