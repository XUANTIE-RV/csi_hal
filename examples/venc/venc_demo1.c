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
#define LOG_PREFIX "venc_demo1"
#include <syslog.h>

#include <csi_venc.h>

#define TEST_MODULE_NAME "simulator_venc0"

int main(int argc, char *argv[])
{
	int ret;
	csi_venc_info_s venc_info;
	csi_venc_dev_t encoder;
	csi_venc_chn_t channel;

	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_venc_get_version(&version);
	printf("Video Encoder HAL version: %d.%d\n", version.major, version.minor);

	// 获取设备中，所有的Video Encoder
	struct csi_venc_infos venc_infos;
	csi_venc_query_list(&venc_infos);

	// 打印所有设备所支持的Video Encoder
	bool found_module = false;
	for (int i = 0; i < venc_infos.count; i++) {
		csi_venc_info_s *info;
		info = &(venc_infos.info[i]);
		printf("decoder[%d]: module_name='%s', device_name='%s', capabilities=0x%lx\n",
		       i, info->module_name, info->device_name, info->capabilities);

		if (strcmp(info->module_name, TEST_MODULE_NAME) == 0) {
			venc_info = *info;
			found_module = true;
		}
	}

	if (!found_module) {
		LOG_E("Can't find module_name:'%s'\n", TEST_MODULE_NAME);
		exit(-1);
	}

	// 打开设备获取句柄，作为后续操对象
	csi_venc_open(&encoder, venc_info.device_name);

	// 创建视频解码通道
	csi_venc_chn_cfg_s chn_cfg;
	chn_cfg.attr.enc_vcodec_id = CSI_VCODEC_ID_H264;
	chn_cfg.attr.max_pic_width = 3840;
	chn_cfg.attr.max_pic_height = 2160;
	chn_cfg.attr.input_format = CSI_PIX_FMT_I420;
	chn_cfg.attr.pic_width = 1920;
	chn_cfg.attr.pic_height = 1080;
	chn_cfg.attr.buf_size = 1*1024*1024;

	chn_cfg.attr.h264_attr.profile = CSI_VENC_H264_PROFILE_HIGH;
	chn_cfg.attr.h264_attr.level = CSI_VENC_H264_LEVEL_4;
	chn_cfg.attr.h264_attr.frame_type = CSI_VCODEC_I_FRAME |
					    CSI_VCODEC_P_FRAME |
					    CSI_VCODEC_B_FRAME;
	chn_cfg.attr.h264_attr.share_buf = true;

	chn_cfg.gop.gop_num = 5;
	chn_cfg.gop.gop_mode = CSI_VENC_GOPMODE_NORMALP;
	chn_cfg.gop.normalp.ip_qp_delta = 10;

	chn_cfg.rc.rc_mode = CSI_VENC_RC_MODE_H264CBR;
	chn_cfg.rc.h264_vbr.stat_time = 10;
	chn_cfg.rc.h264_vbr.framerate_numer = 25;
	chn_cfg.rc.h264_vbr.framerate_denom = 1;
	chn_cfg.rc.h264_vbr.max_bit_rate = 16 * 1024; // kbps
	csi_venc_create_channel(&channel, encoder, &chn_cfg);

	// 配置内存分配器
	//csi_allocator_s *allocator = csi_allocator_get(CSI_ALLOCATOR_TYPE_DMA);
	//csi_venc_set_memory_allocator(channel, allocator);

	// 设置解码通道扩展属性
	csi_venc_chn_ext_property_s ext_property;
	ext_property.prop_id = CSI_VENC_EXT_PROPERTY_ROI;
	ext_property.roi_prop.index = 0;
	ext_property.roi_prop.enable = true;
	ext_property.roi_prop.abs_qp = 1;
	ext_property.roi_prop.qp = 51;
	csi_venc_set_ext_property(channel, &ext_property);

	// 订阅事件
	csi_venc_event_handle_t event_handle;
	csi_venc_create_event_handle(&event_handle, encoder);
	csi_venc_event_subscription_s subscribe_dec, subscribe_chn;

	subscribe_dec.type = CSI_VENC_EVENT_TYPE_DECODER;    // 订阅Encoder事件
	subscribe_dec.id = CSI_VENC_EVENT_ID_ERROR;
	csi_venc_subscribe_event(event_handle, &subscribe_dec);

	subscribe_chn.type = CSI_VENC_EVENT_TYPE_CHANNEL;    // 订阅Channel事件
	subscribe_chn.id = CSI_VENC_CHANNEL_EVENT_ID_FRAME_READY;
	csi_venc_subscribe_event(event_handle, &subscribe_chn);

	// 传送视频码流
	csi_venc_start(channel);

	csi_frame_s frame;
	int send_count = 25;
	for (int i = 0; i < send_count; i++) {
		if (i == 10) {
			// Just for testing
			csi_venc_reset(channel);
		}

		if (i %5 != 0) {
			// Send normal frame
			csi_venc_send_frame(channel, &frame, 1000);
		}
		else {
			// Send frame with extended properties
			csi_venc_frame_prop_s frame_prop[2];
			frame_prop[0].type = CSI_VENC_FRAME_PROP_FORCE_IDR;
			frame_prop[0].force_idr = true;
			frame_prop[1].type = CSI_VENC_FRAME_PROP_FORCE_SKIP;
			frame_prop[1].force_skip = false;
			csi_venc_send_frame_ex(channel, &frame, 1000,
					       frame_prop, ARRAY_SIZE(frame_prop));
		}
	}

	// 收取解码结果
	csi_stream_s stream;
	struct csi_venc_event event;

	int get_count = 0;
	csi_venc_chn_status_s status;

	while (get_count < send_count) {
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
		csi_venc_get_event(event_handle, &event, timeout);

		switch (event.type) {
		case CSI_VENC_EVENT_TYPE_DECODER:
			csi_venc_query_status(channel, &status);
			// Dump Status
			break;
		case CSI_VENC_EVENT_TYPE_CHANNEL:
			switch (event.id) {
			case CSI_VENC_CHANNEL_EVENT_ID_FRAME_READY:
				csi_venc_get_stream(channel, &stream, 1000);
				// save stream ...
				// release stream:  stream.release();
			}
			break;
		default:
			break;
		}
	}

	// 关闭与回收资源
	csi_venc_stop(channel);
	csi_venc_unsubscribe_event(event_handle, &subscribe_dec);
	csi_venc_unsubscribe_event(event_handle, &subscribe_chn);
	csi_venc_destory_channel(channel);
	csi_venc_close(encoder);

	return 0;
}

