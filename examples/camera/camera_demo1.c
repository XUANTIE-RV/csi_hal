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
#define LOG_PREFIX "camera_demo1"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>

#ifdef PLATFORM_SIMULATOR
#include "apputilities.h"
#include "platform_action.h"
#endif

static void dump_camera_meta(csi_frame_s *frame);

//#define TEST_DEVICE_NAME "/dev/fake"
#define TEST_DEVICE_NAME "/dev/video0"

int main(int argc, char *argv[])
{
	bool running = true;
	csi_camera_info_s camera_info;

	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_camera_get_version(&version);
	printf("Camera HAL version: %d.%d\n", version.major, version.minor);

	// 获取设备中，所有的Camera
	struct csi_camera_infos camera_infos;
	csi_camera_query_list(&camera_infos);

	// 打印所有设备所支持的Camera
	for (int i = 0; i < camera_infos.count; i++) {
		camera_info = camera_infos.info[i];
		printf("Camera[%d]: camera_name='%s', device_name='%s', bus_info='%s', capabilities=0x%08x\n",
		       i,
		       camera_info.camera_name, camera_info.device_name, camera_info.bus_info,
		       camera_info.capabilities);
		printf("Camera[%d] caps are:\n",
		       i); /*  The caps are: Video capture, metadata capture */
		for (int j = 1; j <= 0x08000000; j = j << 1) {
			switch (camera_info.capabilities & j) {
			case CSI_CAMERA_CAP_VIDEO_CAPTURE:
				printf("\t camera_infos.info[%d]:Video capture,\n", i);
				break;
			case CSI_CAMERA_CAP_META_CAPTURE:
				printf("\t camera_infos.info[%d] metadata capture,\n", i);
				break;
			default:
				if (camera_info.capabilities & j) {
					printf("\t camera_infos.info[%d] unknown capabilities(0x%08x)\n", i,
					       camera_info.capabilities & j);
				}
				break;
			}
		}
	}
	/*  Camera[0]: camera_name='RGB_Camera', device_name='/dev/video0', bus_info='CSI-MIPI', capabilities=0x00800001
	 *  Camera[1]: camera_name:'Mono_Camera', device_name:'/dev/video8', bus_info='USB', capabilities=0x00000001
	 */

	bool found_camera = false;
	for (int i = 0; i < camera_infos.count; i++) {
		if (strcmp(camera_infos.info[i].device_name, TEST_DEVICE_NAME) == 0) {
			camera_info = camera_infos.info[i];
			printf("found device_name:'%s'\n", camera_info.device_name);
			found_camera = true;
			break;
		}
	}
	if (!found_camera) {
		LOG_E("Can't find camera_name:'%s'\n", TEST_DEVICE_NAME);
		exit(-1);
	}


	// 打开Camera设备获取句柄，作为后续操对象
	csi_cam_handle_t cam_handle;
	csi_camera_open(&cam_handle, camera_info.device_name);

	// 获取Camera支持的工作模式
	struct csi_camera_modes camera_modes;
	csi_camera_get_modes(cam_handle, &camera_modes);

	// 打印camera所支持的所有工作模式
	printf("Camera:'%s' modes are:\n", TEST_DEVICE_NAME);
	printf("{\n");
	for (int i = 0; i < camera_modes.count; i++) {
		printf("\t mode_id=%d: description:'%s'\n",
		       camera_modes.modes[i].mode_id, camera_modes.modes[i].description);
	}
	printf("}\n");

	// 设置camera的工作模式及其配置
	csi_camera_mode_cfg_s camera_cfg;
	camera_cfg.mode_id = 1;
	camera_cfg.calibriation = NULL; // 采用系统默认配置
	camera_cfg.lib3a = NULL;	// 采用系统默认配置
	csi_camera_set_mode(cam_handle, &camera_cfg);

	// 获取单个可控单元的属性
	csi_camera_property_description_s description;
	description.id = CSI_CAMERA_PID_HFLIP;
	csi_camera_query_property(cam_handle, &description);
	printf("id=0x%08x type=%d default=%d value=%d\n",
	       description.id, description.type,
	       description.default_value.bool_value, description.value.bool_value);
	/* id=0x0098090x, type=2 default=0 value=1 */
	/* Other example:
	 * id=0x0098090y, type=3 min=0 max=255 step=1 default=127 value=116
	 * id=0x0098090z, type=4 min=0 max=3 default=0 value=2
	 *                                0: IDLE
	 *                                1: BUSY
	 *                                2: REACHED
	 *                                3: FAILED
	 */

	// 轮询获取所有可控制的单元
	printf("all properties are:\n");
	description.id = CSI_CAMERA_PID_HFLIP;
	while (!csi_camera_query_property(cam_handle, &description)) {
		switch (description.type) {
		case (CSI_CAMERA_PROPERTY_TYPE_INTEGER):
			printf("id=0x%08x type=%d default=%d value=%d\n",
			       description.id, description.type,
			       description.default_value.int_value, description.value.int_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BOOLEAN):
			printf("id=0x%08x type=%d default=%d value=%d\n",
			       description.id, description.type,
			       description.default_value.bool_value, description.value.bool_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_ENUM):
			printf("id=0x%08x type=%d default=%d value=%d\n",
			       description.id, description.type,
			       description.default_value.enum_value, description.value.enum_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_STRING):
			printf("id=0x%08x type=%d default=%s value=%s\n",
			       description.id, description.type,
			       description.default_value.str_value, description.value.str_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BITMASK):
			printf("id=0x%08x type=%d default=%x value=%x\n",
			       description.id, description.type,
			       description.default_value.bitmask_value, description.value.bitmask_value);
			break;
		default:
			LOG_E("error type!\n");
			break;
		}
		description.id |= CSI_CAMERA_FLAG_NEXT_CTRL;
	}

	// 同时配置多个参数
	csi_camera_properties_s properties;
	csi_camera_property_s property[2];
	property[0].id = CSI_CAMERA_PID_HFLIP;
	property[0].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[0].value.bool_value = true;
	property[1].id = CSI_CAMERA_PID_VFLIP;
	property[1].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[1].value.bool_value = false;
	properties.count = 2;
	properties.property = property;
	csi_camera_set_property(cam_handle, &properties);
	LOG_O("set_property ok!\n");

	csi_camera_get_property(cam_handle, &properties);



	// 查询输出channel
	csi_camera_channel_cfg_s chn_cfg;
	chn_cfg.chn_id = CSI_CAMERA_CHANNEL_0;
	csi_camera_channel_query(cam_handle, &chn_cfg);
	if (chn_cfg.status != CSI_CAMERA_CHANNEL_CLOSED) {
		printf("Can't open CSI_CAMERA_CHANNEL_0\n");
		exit(-1);
	}

	// 打开输出channel
	chn_cfg.chn_id = CSI_CAMERA_CHANNEL_0;
	chn_cfg.frm_cnt = 4;
	chn_cfg.img_fmt.width =  1280;
	chn_cfg.img_fmt.height = 720;
	chn_cfg.img_fmt.pix_fmt = CSI_PIX_FMT_BGR;
	chn_cfg.img_type = CSI_IMG_TYPE_DMA_BUF;
	chn_cfg.meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
	chn_cfg.capture_type = CSI_CAMERA_CHANNEL_CAPTURE_VIDEO |
			       CSI_CAMERA_CHANNEL_CAPTURE_META;
	csi_camera_channel_open(cam_handle, &chn_cfg);

	// 订阅Event
	csi_cam_event_handle_t event_handle;
	csi_camera_create_event(&event_handle, cam_handle);
	csi_camera_event_subscription_s subscribe;
	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CAMERA;      // 订阅Camera的ERROR事件
	subscribe.id = CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
	csi_camera_subscribe_event(event_handle, &subscribe);
	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CHANNEL0;    // 订阅Channel0的FRAME_READY事件
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe);

	// 开始从channel中取出准备好的frame
	csi_camera_channel_start(cam_handle, CSI_CAMERA_CHANNEL_0);

	// 处理订阅的Event
	csi_frame_s frame;
	struct csi_camera_event event;

	while (running) {
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
		csi_camera_get_event(event_handle, &event, timeout);

		switch (event.type) {
		case CSI_CAMERA_EVENT_TYPE_CAMERA:
			switch (event.id) {
			case CSI_CAMERA_EVENT_ERROR:
				// do sth.
				LOG_D("get CAMERA EVENT CSI_CAMERA_EVENT_ERROR!\n");
				break;
			default:
				break;
			}
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
			switch (event.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
				int read_frame_count = csi_camera_get_frame_count(cam_handle,
						       CSI_CAMERA_CHANNEL_0);
				for (int i = 0; i < read_frame_count; i++) {
					csi_camera_get_frame(cam_handle, CSI_CAMERA_CHANNEL_0, &frame, timeout);

					// frame.image: {.width, .height, .pix_fmt, .dma-buf}
					//show_frame_image(frame.image); // 伪代码
#ifdef PLATFORM_SIMULATOR
					camera_action_image_display(&frame);
					csi_camera_put_frame(&frame);
#endif

					// frame.meta: {.count,
					//              .meta_data[]={meta_id, type,
					//                            union{int_value, str_value, ...}}}

					dump_camera_meta(&frame);

					//csi_camera_put_frame(&frame);
				}
				break;
			}
			default:
				break;
			}
		default:
			break;
		}
	}

	// 取消订阅某一个event, 也可以直接调用csi_camera_destory_event，结束所有的订阅
	subscribe.type = CSI_CAMERA_EVENT_TYPE_CHANNEL0;
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	csi_camera_destory_event(event_handle);
	csi_camera_channel_stop(cam_handle, CSI_CAMERA_CHANNEL_0);
	csi_camera_channel_close(cam_handle, CSI_CAMERA_CHANNEL_0);
	csi_camera_close(cam_handle);
}

static void dump_camera_meta(csi_frame_s *frame)
{
	int i;
	if (frame->meta.type != CSI_META_TYPE_CAMERA)
		return;

	csi_camera_meta_s *meta_data = (csi_camera_meta_s *)frame->meta.data;
	int meta_count = meta_data->count;
	csi_camrea_meta_unit_s meta_unit;

	for (i = 0; i < meta_count; i++) {
		csi_camera_frame_get_meta_unit(
			&meta_unit, meta_data, CSI_CAMERA_META_ID_FRAME_ID);
		printf("meta_id=%d, meta_type=%d, meta_value=%d",
		       meta_unit.id, meta_unit.type, meta_unit.int_value);
	}
}

