/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: zhuxinran <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

#ifndef _DRV_RINGBUFFER_H_
#define _DRV_RINGBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include <stdbool.h>

typedef struct ringbuffer {
	uint8_t *buffer;
	uint32_t size;
	uint32_t write;
	uint32_t read;
	uint32_t data_len;
} ringbuffer_t;

void ringbuffer_reset(ringbuffer_t *fifo);
uint32_t ringbuffer_len(ringbuffer_t *fifo);
uint32_t ringbuffer_avail(ringbuffer_t *fifo);
bool ringbuffer_is_empty(ringbuffer_t *fifo);
bool ringbuffer_is_full(ringbuffer_t *fifo);

/*write to ringbuffer*/
uint32_t ringbuffer_in(ringbuffer_t *fifo, const void *in, uint32_t len);

/*read to ringbuffer*/
uint32_t ringbuffer_out(ringbuffer_t *fifo, void *out, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_RINGBUFFER_H_ */
