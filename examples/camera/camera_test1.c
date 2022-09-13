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

#define LOG_LEVEL 2
#define LOG_PREFIX "camera_demo1"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>

#ifdef PLATFORM_SIMULATOR
#include "apputilities.h"
#endif

static void dump_camera_meta(csi_frame_s *frame);
static int save_camera_img(csi_frame_s *frame);
static int save_camera_stream(csi_frame_s *frame);

#define TEST_DEVICE_NAME "/dev/video0"
#define CSI_CAMERA_TRUE  1
#define CSI_CAMERA_FALSE 0

char *prog_name; 

void usage()
{
	fprintf (stderr,
			"usage: %s [options]\n"
			"Options:\n"
			"\t-D dev		target video device like /dev/video0\n"
			"\t-R resolution        format is like 640x480\n"
			"\t-F format		like NV12\n"
			"\t-C channel_index		channel index defined in dts, index from 0\n"
			"\t-M output mode		0: save as image; 1: save as stream file\n"
			"\t-N number for frames		0: record forever; others: the recorded frame numbers"
			, prog_name
		);
}

int main(int argc, char *argv[])
{
	bool running = false;
	csi_camera_info_s camera_info;

	int opt;
	char device[CSI_CAMERA_NAME_MAX_LEN];    /* target video device like /dev/video0 */
	char *resolution;    /* resolution information like 640x480 */
	int hres, vres;    /* resolution information like 640x480 */
	char *resDelim = "xX";
	char *format;    /* format information like NV12 */
	enum csi_pixel_fmt fenum;
	int channelIdx;    /* channel information like 8 */
	int outMode = 0;
	int fNum = 0;

	prog_name = argv[0];

	while ((opt = getopt(argc, argv, "D:R:F:C:M:hN:")) != EOF) {
		switch(opt) {
			case 'D':
				//device = optarg;
				strcpy(device, optarg);
				continue;
			case 'R':
				resolution = optarg;
				hres = atoi(strtok(resolution, resDelim));
				vres = atoi(strtok(NULL, resDelim));
				continue;
			case 'M':
				outMode = atoi(optarg);
				continue;
			case 'N':
				fNum = atoi(optarg);
				continue;
			case 'F':
				format = optarg;
				if(strstr(format, "NV12")){
					fenum = CSI_PIX_FMT_NV12;
				}
				else if(strstr(format, "I420")){
					fenum = CSI_PIX_FMT_I420;
				}
				else{
					fenum = CSI_PIX_FMT_I420;
				}
				continue;
			case 'C':
				channelIdx = atoi(optarg);
				continue;
			case '?':
			case 'h':
			default:
				usage();
				return 1;
		}
	}

	const csi_camera_channel_id_e CAMERA_CHANNEL_ID = CSI_CAMERA_CHANNEL_0 + channelIdx;
	const csi_camera_event_type_e CAMERA_CHANNEL_EVENT_TYPE = CSI_CAMERA_EVENT_TYPE_CHANNEL0 + channelIdx;
	if (fNum == 0)
		running == true;


	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_camera_get_version(&version);
	printf("Camera HAL version: %d.%d\n", version.major, version.minor);

	// 获取设备中，所有的Camera
	struct csi_camera_infos camera_infos;
	memset(&camera_infos, 0, sizeof(camera_infos));
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


	// 打开Camera设备获取句柄，作为后续操对象
	csi_cam_handle_t cam_handle;
	//csi_camera_open(&cam_handle, camera_info.device_name);
	strcpy(camera_info.device_name, device);
	//camera_info.device_name = device;
	csi_camera_open(&cam_handle, camera_info.device_name);
	//csi_camera_open(&cam_handle, device);
	// 获取Camera支持的工作模式
	struct csi_camera_modes camera_modes;
	csi_camera_get_modes(cam_handle, &camera_modes);

	// 打印camera所支持的所有工作模式
	printf("Camera:'%s' modes are:\n", device);
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
	csi_camera_property_s property[3];
	property[0].id = CSI_CAMERA_PID_HFLIP;
	property[0].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[0].value.bool_value = false;
	property[1].id = CSI_CAMERA_PID_VFLIP;
	property[1].type = CSI_CAMERA_PROPERTY_TYPE_BOOLEAN;
	property[1].value.bool_value = false;
	property[2].id = CSI_CAMERA_PID_ROTATE;
	property[2].type = CSI_CAMERA_PROPERTY_TYPE_INTEGER;
	property[2].value.int_value = 0;
	properties.count = 3;
	properties.property = property;
	if (csi_camera_set_property(cam_handle, &properties) < 0) {
		LOG_O("set_property fail!\n");
	}
	LOG_O("set_property ok!\n");

	// 查询输出channel
	csi_camera_channel_cfg_s chn_cfg;
	chn_cfg.chn_id = CAMERA_CHANNEL_ID;
	csi_camera_channel_query(cam_handle, &chn_cfg);
	if (chn_cfg.status != CSI_CAMERA_CHANNEL_CLOSED) {
		printf("Can't open CSI_CAMERA_CHANNEL_0\n");
		exit(-1);
	}

	// 打开输出channel
	chn_cfg.chn_id = channelIdx;
	chn_cfg.frm_cnt = 4;
	chn_cfg.img_fmt.width = hres;
	chn_cfg.img_fmt.height = vres;
	chn_cfg.img_fmt.pix_fmt = fenum;
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
		CAMERA_CHANNEL_EVENT_TYPE;    // 订阅Channel0的FRAME_READY事件
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe);

	// 开始从channel中取出准备好的frame
	csi_camera_channel_start(cam_handle, CAMERA_CHANNEL_ID);

	// 处理订阅的Event
	csi_frame_s frame;
	struct csi_camera_event event;

	while (running || fNum >= 0 ) {
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
		csi_camera_get_event(event_handle, &event, timeout);
		if(event.type == CSI_CAMERA_EVENT_TYPE_CAMERA){
			switch (event.id) {
				case CSI_CAMERA_EVENT_ERROR:
					// do sth.
					LOG_D("get CAMERA EVENT CSI_CAMERA_EVENT_ERROR!\n");
					break;
				default:
					break;
			}
		}
		if(event.type == CAMERA_CHANNEL_EVENT_TYPE){
			switch (event.id) {
				case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
					   int read_frame_count = csi_camera_get_frame_count(cam_handle,
							   CAMERA_CHANNEL_ID);
					   for (int i = 0; i < read_frame_count; i++) {
						   csi_camera_get_frame(cam_handle, CAMERA_CHANNEL_ID, &frame, timeout);
						   fNum--;

#ifdef PLATFORM_SIMULATOR
						   show_frame_image(frame.img.usr_addr[0], frame.img.height, frame.img.width);
#endif

						   if (outMode == 0)
							   save_camera_img(&frame);
						   else
							   save_camera_stream(&frame);

						   csi_frame_release(&frame);
					   }
					   break;
				   }
				default:
					   break;
			}
		}
	}
	csi_camera_channel_stop(cam_handle, CAMERA_CHANNEL_ID);
	usleep (1000000);
	// 取消订阅某一个event, 也可以直接调用csi_camera_destory_event，结束所有的订阅
	subscribe.type = CAMERA_CHANNEL_EVENT_TYPE;
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	csi_camera_destory_event(event_handle);

	csi_camera_channel_close(cam_handle, CAMERA_CHANNEL_ID);
	csi_camera_close(cam_handle);
}

static void dump_camera_meta(csi_frame_s *frame)
{
	int i;
	//printf("%s\n", __func__);
	if (frame->meta.type != CSI_META_TYPE_CAMERA)
		return;

	csi_camera_meta_s *meta_data = (csi_camera_meta_s *)frame->meta.data;
	int meta_count = meta_data->count;
	csi_camrea_meta_unit_s meta_unit;

	for (i = 0; i < meta_count; i++) {
		csi_camera_frame_get_meta_unit(
				&meta_unit, meta_data, CSI_CAMERA_META_ID_FRAME_ID);
		//printf("meta_id=%d, meta_type=%d, meta_value=%d",
		//       meta_unit.id, meta_unit.type, meta_unit.int_value);
	}
}

static int save_camera_img(csi_frame_s *frame)
{
	static int fcount = 0;
	char fname[20]; 
	FILE *fp;
	fcount = fcount%4;
	sprintf(fname, "%s%d%s", "hal_image", fcount, ".yuv");
	fcount++;

	if((fp = fopen(fname, "wb")) == NULL){
		printf("Error: Can't open file\n");
		return -1;
	}
	fwrite(frame->img.usr_addr[0], sizeof(char), frame->img.size, fp);
	fclose(fp);
	return 0;
}

static int save_camera_stream(csi_frame_s *frame)
{
	FILE *fp;
	char *fname = "hal_stream.yuv";
	if((fp = fopen(fname, "ab+")) == NULL){
		printf("Error: Can't open file\n");
		return -1;
	}
	fwrite(frame->img.usr_addr[0], sizeof(char), frame->img.size, fp);
	fclose(fp);
	return 0;
}

