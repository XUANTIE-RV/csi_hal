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
#include <errno.h>
#include <pthread.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "camera_demo4"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>

#define TEST_DEVICE_NAME "/dev/video0"

typedef struct cam_channel_control {
	pthread_t thread_id;
	bool run;
	csi_cam_handle_t cam_handle;
	csi_camera_channel_cfg_s chn_cfg;
} cam_channel_control_t;

static void dump_camera_meta(csi_frame_s *frame);
static void *channel_thread(void *arg);

int main(int argc, char *argv[])
{
	bool running = true;
	csi_camera_info_s camera_info;

	// 获取设备中，所有的Camera
	struct csi_camera_infos camera_infos;
	csi_camera_query_list(&camera_infos);
	LOG_O("csi_camera_query_list() OK\n");

	// 打印所有设备所支持的Camera
	for (uint32_t i = 0; i < camera_infos.count; i++) {
		printf("Camera[%d]: camera_name='%s', device_name='%s', bus_info='%s', capabilities=0x%08x\n",
		       i, camera_infos.info[i].camera_name, camera_infos.info[i].device_name,
		       camera_infos.info[i].bus_info, camera_infos.info[i].capabilities);
	}
	/*  Camera[0]: camera_name='RGB_Camera', device_name='/dev/video0', bus_info='CSI-MIPI', capabilities=0x00800001
	 *  Camera[1]: camera_name:'Mono_Camera', device_name:'/dev/video8', bus_info='USB', capabilities=0x00000001
	 */

	bool found_camera = false;
	for (uint32_t i = 0; i < camera_infos.count; i++) {
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

	printf("The caps are:\n");
	for (uint32_t i = 1; i < 0x80000000; i = i << 1) {
		switch (camera_info.capabilities & i) {
		case CSI_CAMERA_CAP_VIDEO_CAPTURE:
			printf("\t Video capture\n");
			break;
		case CSI_CAMERA_CAP_META_CAPTURE:
			printf("\t metadata capture\n");
			break;
		default:
			if (camera_info.capabilities & i) {
				printf("\t unknown capabilities(0x%08x)\n", camera_info.capabilities & i);
			}
			break;
		}
	}
	/*  The caps are: Video capture, metadata capture */

	// 打开Camera设备获取句柄，作为后续操对象
	csi_cam_handle_t cam_handle;
	csi_camera_open(&cam_handle, camera_info.device_name);
	LOG_O("csi_camera_open() OK\n");

	// 获取Camera支持的工作模式
	struct csi_camera_modes camera_modes;
	camera_modes.count = 0;
	csi_camera_get_modes(cam_handle, &camera_modes);
	LOG_O("csi_camera_get_modes() OK\n");

	// 打印camera所支持的所有工作模式
	printf(" Camera:'%s' modes are:{\n", TEST_DEVICE_NAME);
	for (uint32_t i = 0; i < camera_modes.count; i++) {
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
	printf("properity id=0x%08x type=%d default=%d value=%d\n",
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
	description.id = CSI_CAMERA_PID_HFLIP;
	while (!csi_camera_query_property(cam_handle, &description)) {
		//printf(...); // 打印属性
		description.id |= CSI_CAMERA_FLAG_NEXT_CTRL;
	}
	LOG_O("csi_camera_query_property() OK\n");

	// 同时配置多个参数
	csi_camera_properties_s properties;
	csi_camera_property_s property[2];
	property[0].id = CSI_CAMERA_PID_HFLIP;
	property[0].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[0].value.bool_value = true;
	property[0].id = CSI_CAMERA_PID_VFLIP;
	property[0].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[0].value.bool_value = true;
	properties.count = 2;
	properties.property = property;
	csi_camera_get_property(cam_handle, &properties);
	LOG_O("csi_camera_get_property() OK\n");

	cam_channel_control_t channel_control[2];

	for (int i = 0; i < 2; i++) {
		cam_channel_control_t *chn_ctrl = &channel_control[i];
		pthread_t *thread_id = &(chn_ctrl->thread_id);
		csi_camera_channel_cfg_s *chn_cfg = &(chn_ctrl->chn_cfg);
		chn_ctrl->run = true;
		chn_ctrl->cam_handle = cam_handle;

		chn_cfg->frm_cnt = 4;
		if (i = 0) {
			chn_cfg->chn_id = CSI_CAMERA_CHANNEL_0;
			chn_cfg->img_fmt.width = 640;
			chn_cfg->img_fmt.height = 480;
			chn_cfg->img_fmt.pix_fmt = CSI_PIX_FMT_NV12;
		} else {
			chn_cfg->chn_id = CSI_CAMERA_CHANNEL_1;
			chn_cfg->img_fmt.width = 1280;
			chn_cfg->img_fmt.height = 720;
			chn_cfg->img_fmt.pix_fmt = CSI_PIX_FMT_I420;
		}
		chn_cfg->meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
		chn_cfg->capture_type = CSI_CAMERA_CHANNEL_CAPTURE_VIDEO |
					CSI_CAMERA_CHANNEL_CAPTURE_META;

		pthread_create(thread_id, NULL, channel_thread, chn_ctrl);
		sleep(1);	// channel[1] start after 1 second
	}
	sleep(3);
	channel_control[0].run = false;
	sleep(2);
	channel_control[1].run = false;

	for (int i = 0; i < 2; i++) {
		cam_channel_control_t *chn_ctrl = &channel_control[i];
		int chn_id = chn_ctrl->chn_cfg.chn_id;
		if (pthread_join(chn_ctrl->thread_id, NULL)) {
			LOG_E("[chn-%d] pthread_join() failed, %s\n", chn_id, strerror(errno));
		}
	}

	csi_camera_close(cam_handle);
	return 0;
}

static void *channel_thread(void *arg)
{
	int ret;
	cam_channel_control_t *chn_ctrl = (cam_channel_control_t *)arg;
	csi_cam_handle_t cam_handle = chn_ctrl->cam_handle;
	int chn_id = chn_ctrl->chn_cfg.chn_id;

	ret = csi_camera_channel_open(cam_handle, &(chn_ctrl->chn_cfg));
	if (ret != 0) {
		LOG_E("[chn_%d] csi_camera_channel_open() failed, ret=%d\n", chn_id, ret);
		return NULL;
	}
	LOG_O("[chn_%d] csi_camera_channel_open() OK\n", chn_id);

	// 订阅Event
	csi_cam_event_handle_t event_handle;
	csi_camera_create_event(&event_handle, cam_handle);
	struct csi_camera_event_subscription subscribe_cam;
	struct csi_camera_event_subscription subscribe_chn;
	subscribe_cam.type = CSI_CAMERA_EVENT_TYPE_CAMERA; // 订阅Camera的ERROR事件
	subscribe_cam.id = CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
	csi_camera_subscribe_event(event_handle, &subscribe_cam);

	subscribe_chn.type = chn_id;    // 订阅Channel0的FRAME_READY事件
	subscribe_chn.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		           CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe_chn);
	LOG_O("Event subscript OK\n");

	// 处理订阅的Event
	LOG_O("[chn_%d] Starting get events...\n", chn_id);
	csi_frame_s frame;
	struct csi_camera_event event;

	while (chn_ctrl->run) {
		int timeout = 100; // unit: ms, -1 means wait forever, or until error occurs
		LOG_D("[chn_%d] before csi_camera_get_event\n", chn_id);
		csi_camera_get_event(event_handle, &event, timeout);
		LOG_D("[chn_%d] after csi_camera_get_event\n", chn_id);

		switch (event.type) {
		case CSI_CAMERA_EVENT_TYPE_CAMERA:
			switch (event.id) {
			case CSI_CAMERA_EVENT_ERROR:
				// do sth.
				break;
			default:
				break;
			}
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
			if (event.type != chn_id) {
				LOG_E("[chn_%d] Recv type(%d) is not expected\n",
					chn_id, event.type);
				break;
			}
			switch (event.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
				LOG_O("[chn_%d] Get ready frame\n", chn_id);
				int read_frame_count = csi_camera_get_frame_count(cam_handle, chn_id);
				for (int i = 0; i < read_frame_count; i++) {
					csi_camera_get_frame(cam_handle, chn_id, &frame, timeout);
					dump_camera_meta(&frame);
					csi_camera_put_frame(&frame);
				}
				break;
			}
			default:
				LOG_W("[chn_%d] Get unknown event type\n", event.type);
				break;
			}
		default:
			break;
		}
	}

	csi_camera_unsubscribe_event(event_handle, &subscribe_cam);
	csi_camera_unsubscribe_event(event_handle, &subscribe_chn);
	csi_camera_destory_event(event_handle);
	csi_camera_channel_stop(cam_handle, chn_id);
	pthread_exit(NULL);
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

