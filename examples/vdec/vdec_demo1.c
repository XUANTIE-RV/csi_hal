/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "vdec_demo1"
#include <syslog.h>

#include <csi_vdec.h>
#include <csi_allocator.h>

#define TEST_MODULE_NAME "simulator_vdec0"

int main(int argc, char *argv[])
{
	int ret;
	csi_vdec_info_t vdec_info;
	csi_vdec_dev_t decoder;
	csi_vdec_chn_t channel;

	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_vdec_get_version(&version);
	printf("Video Decoder HAL version: %d.%d\n", version.major, version.minor);

	// 获取设备中，所有的Video Decoder
	struct csi_vdec_infos vdec_infos;
	csi_vdec_query_list(&vdec_infos);

	// 打印所有设备所支持的Video Decoder
	bool found_module = false;
	for (int i = 0; i < vdec_infos.count; i++) {
		csi_vdec_info_t *info;
		info = &(vdec_infos.info[i]);
		printf("decoder[%d]: module_name='%s', device_name='%s', capabilities=0x%lx\n",
		       i, info->module_name, info->device_name, info->capabilities);

		if (strcmp(info->module_name, TEST_MODULE_NAME) == 0) {
			vdec_info = *info;
			found_module = true;
		}
	}

	if (!found_module) {
		LOG_E("Can't find module_name:'%s'\n", TEST_MODULE_NAME);
		exit(-1);
	}

	// 打开设备获取句柄，作为后续操对象
	csi_vdec_open(&decoder, vdec_info.device_name);

	// 创建视频解码通道
	csi_vdec_config_s dec_cfg;
	dec_cfg.input_mode = CSI_VDEC_INPUT_MODE_FRAME;
	dec_cfg.input_stream_buf_size = 1*1024*1024;
	dec_cfg.output_format = CSI_PIX_FMT_I420;
	dec_cfg.output_width = 1920;
	dec_cfg.output_height = 1080;
	dec_cfg.output_order = CSI_VDEC_OUTPUT_ORDER_DISP;
	dec_cfg.dec_vcodec_id = CSI_VCODEC_ID_H264;
	dec_cfg.output_img_type = CSI_VDEC_MODE_IPB;
	dec_cfg.dec_frame_buf_cnt = 8;
	csi_vdec_create_channel(&channel, decoder, &dec_cfg);

	// 配置内存分配器
	if (1) {
	    //csi_allocator_s *allocator = csi_allocator_get(CSI_ALLOCATOR_TYPE_DMA);
	    //csi_vdec_set_memory_allocator(channel, allocator);
	} else {
	    csi_frame_s *allocated_frames[8];   // TODO: allocate frames first
	    csi_vdec_register_frames(channel, allocated_frames, 8);
	}

	// 配置解码通道模式
	csi_vdec_mode_s vdec_mode;
	vdec_mode.fb_source = CSI_FB_SOURCE_DMABUF;
	vdec_mode.low_latency_mode = false;
	vdec_mode.mini_buf_mode = false;
	csi_vdec_set_mode(channel, &vdec_mode);

	// 获取和重新配置解码通道属性
	csi_vdec_get_chn_config(channel, &dec_cfg);
	dec_cfg.dec_frame_buf_cnt = 16;
	csi_vdec_set_chn_config(channel, &dec_cfg);

	// 订阅事件
	csi_vdec_event_handle_t event_handle;
	csi_vdec_create_event_handle(&event_handle, decoder);
	csi_vdec_event_subscription_t subscribe_dec, subscribe_chn;

	subscribe_dec.type = CSI_VDEC_EVENT_TYPE_DECODER;    // 订阅Decoder事件
	subscribe_dec.id = CSI_VDEC_EVENT_ID_ERROR;
	csi_vdec_subscribe_event(event_handle, &subscribe_dec);

	subscribe_chn.type = CSI_VDEC_EVENT_TYPE_CHANNEL;    // 订阅Channel事件
	subscribe_chn.id = CSI_VDEC_CHANNEL_EVENT_ID_FRAME_READY;
	csi_vdec_subscribe_event(event_handle, &subscribe_chn);

	// 传送视频码流
	csi_vdec_start(channel);

	const int send_count = 25;
	const int stream_packet_len = 1024;

	csi_vdec_stream_s stream;
	stream.data = malloc(stream_packet_len);

	for (int i = 0; i < send_count; i++) {
		stream.length = stream_packet_len;
		stream.pts = 40 * i;
		stream.eos = (i < (send_count - 1)) ? false : true;

		if (i == 10) {
			// Just for testing
			csi_vdec_reset(channel);
		}
		csi_vdec_send_stream_buf(channel, &stream, 1000);
	}
	free(stream.data);
	stream.data = NULL;

	// 收取解码结果
	csi_frame_s *frame;
	struct csi_vdec_event event;

	int get_count = 0;
	csi_vdec_chn_status_s status;

	while (get_count < send_count) {
		// timeout unit: ms, -1 means wait forever, or until error occurs
		csi_vdec_get_event(event_handle, &event, -1);

		switch (event.type) {
		case CSI_VDEC_EVENT_TYPE_DECODER:
			csi_vdec_query_status(channel, &status);
			// Dump Status
			break;
		case CSI_VDEC_EVENT_TYPE_CHANNEL:
			switch (event.id) {
			case CSI_VDEC_CHANNEL_EVENT_ID_FRAME_READY:
				csi_vdec_get_frame(channel, &frame, 1000);
				get_count++;
				// display frame
				// release frame: frame.release();
			}
			break;
		default:
			break;
		}


	}

	// 关闭与回收资源
	csi_vdec_stop(channel);
	csi_vdec_unsubscribe_event(event_handle, &subscribe_dec);
	csi_vdec_unsubscribe_event(event_handle, &subscribe_chn);
	csi_vdec_destory_channel(channel);
	csi_vdec_close(decoder);

	return 0;
}

