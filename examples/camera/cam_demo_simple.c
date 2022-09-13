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
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#define LOG_LEVEL 2
#define LOG_PREFIX "cam_demo_simple"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>
#include "csi_camera_dev_api.h"
// #include "vi_mem.h"
#ifdef PLATFORM_SIMULATOR
#include "apputilities.h"
#endif

static void dump_camera_meta(csi_frame_s *frame);
static void dump_camera_frame(csi_frame_s *frame, csi_pixel_fmt_e fmt);


#define TEST_DEVICE_NAME "/dev/video0"
#define CSI_CAMERA_TRUE  1
#define CSI_CAMERA_FALSE 0

typedef struct frame_fps {
	uint64_t frameNumber;
	struct timeval initTick;
	float fps;
} frame_fps_t;

typedef struct cb_context{
    int id;
    csi_cam_handle_t cam_handle;
    int dsp_id;
    int dsp_path;
    void *ref_buf;
    int buf_size;
}cb_context_t;

typedef struct cb_args{
    char setting[16];
    uint32_t frame_id;
    uint32_t timestap;
}cb_args_t;
cb_context_t cb_context[2];

void dsp_algo_result_cb(void*context,void*arg)
{
    cb_context_t *ctx = (cb_context_t *)context;
    cb_args_t *cb_args= (cb_args_t*)arg;
    char update_setting[16];
    uint32_t frame_id = cb_args->frame_id;
    uint32_t frame_timestap =  cb_args->timestap;
    LOG_O("cb:%d,setting:%s,frame :%d,timestad:0x%x\n",ctx->id,cb_args->setting,frame_id,frame_timestap);
    sprintf(update_setting, "update_%d", frame_id);
    csi_camera_update_dsp_algo_setting(ctx->cam_handle,ctx->dsp_id,ctx->dsp_path,update_setting);
    if(frame_id%20)
    {
        void *replace_buf;
        csi_camera_update_dsp_algo_buf(ctx->cam_handle, ctx->dsp_id,ctx->dsp_path,ctx->ref_buf,&replace_buf);
        LOG_O("cb:%d,old buffer:0x%lx,new buffer:0x%lx\n",ctx->id,(uint64_t)ctx->ref_buf,(uint64_t)replace_buf);
        ctx->ref_buf = replace_buf;
    }
}


static void usage(void)
{
	printf("    1 : video id  cases\n");
	printf("    2 : channel cases\n");
	printf("    3 : dump enable\n");
	printf("    4 : display enable\n");
	printf("    5 : width\n");
	printf("    6 : heithg\n");
	printf("    7 : fmt\n");
	printf("    8 : frame_num\n");
	printf("    9 : file_id\n");
}

static void get_system_time(const char *func, int line_num)
{
	struct timeval cur_time;

	memset(&cur_time, 0, sizeof(cur_time));
	gettimeofday(&cur_time, 0);
	LOG_O("%s %s line_num = %d, cur_time.tv_sec = %ld, cur_time.tv_usec = %ld\n",
		__func__, func, line_num, cur_time.tv_sec, cur_time.tv_usec);
}


int file_id = 0;
extern void *vi_plink_create(csi_camera_channel_cfg_s *chn_cfg);
extern void vi_plink_release(void * plink);
extern void display_camera_frame(void * plink, csi_frame_s *frame);
int main(int argc, char *argv[])
{
	bool running = true;
	int channel_id = -1;
	int video_id = -1;
	int dump_enable = -1;
	int display_enable = -1;
	int width = 0, height = 0, fmt = -1;
	int ret = 0;
	uint64_t frame_num = 0;
	csi_camera_info_s camera_info;
	memset(&camera_info, 0, sizeof(camera_info));
	int chn_id = CSI_CAMERA_CHANNEL_0;
	int env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL0;
	frame_fps_t demo_fps;
	memset(&demo_fps, 0, sizeof(demo_fps));
	struct timeval init_time, cur_time;
	memset(&init_time, 0, sizeof(init_time));
	memset(&cur_time, 0, sizeof(cur_time));
	get_system_time(__func__, __LINE__);
	char dev_name[128];
	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_camera_get_version(&version);
	if (argc == 5)
	{
		video_id = atoi(argv[1]);
		channel_id = atoi(argv[2]);
		dump_enable = atoi(argv[3]);
		display_enable = atoi(argv[4]);
		width = 640;
		height = 480;
		fmt = 1;
		printf("%s, %s, %s, %s \n", argv[1],argv[2],argv[3],argv[4]);
		printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$, %d, display_enable is %d\n", __LINE__, display_enable );
	} else if (argc == 8) {
		video_id = atoi(argv[1]);
		channel_id = atoi(argv[2]);
		dump_enable = atoi(argv[3]);
		display_enable = atoi(argv[4]);
		width = atoi(argv[5]);
		height = atoi(argv[6]);
		fmt = atoi(argv[7]);
	}else if (argc == 9) {
		video_id = atoi(argv[1]);
		channel_id = atoi(argv[2]);
		dump_enable = atoi(argv[3]);
		display_enable = atoi(argv[4]);
		width = atoi(argv[5]);
		height = atoi(argv[6]);
		fmt = atoi(argv[7]);
		frame_num = atoi(argv[8]);
	}else if (argc == 10) {
		video_id = atoi(argv[1]);
		channel_id = atoi(argv[2]);
		dump_enable = atoi(argv[3]);
		display_enable = atoi(argv[4]);
		width = atoi(argv[5]);
		height = atoi(argv[6]);
		fmt = atoi(argv[7]);
		frame_num = atoi(argv[8]);
		file_id = atoi(argv[9]);
	} else {
		usage();
		video_id = 0;
		channel_id = 0;
		dump_enable = 0;
		display_enable = 0;
		width = 640;
		height = 480;
		fmt = 1;
		printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$, %d, display_enable is %d\n", __LINE__, display_enable );
	}
	sprintf(dev_name, "/dev/video%d", video_id);
	LOG_O("Camera HAL version: %d.%d video_id = %d channel_id = %d size{%d, %d} fmt = %d\n", version.major, version.minor, video_id, channel_id, width, height, fmt);

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
		printf("Camera[%d] caps are:\n", i); /*  The caps are: Video capture, metadata capture */
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
		if (strcmp(camera_infos.info[i].device_name, dev_name) == 0) {
			camera_info = camera_infos.info[i];
			printf("found device_name:'%s'\n", camera_info.device_name);
			found_camera = true;
			break;
		}
	}
	if (!found_camera) {
		LOG_E("Can't find camera_name:'%s'\n", dev_name);
		exit(-1);
	}
	get_system_time(__func__, __LINE__);
	// 打开Camera设备获取句柄，作为后续操对象
	csi_cam_handle_t cam_handle;
	csi_camera_open(&cam_handle, camera_info.device_name);
	get_system_time(__func__, __LINE__);
	// 获取Camera支持的工作模式
	struct csi_camera_modes camera_modes;
	csi_camera_get_modes(cam_handle, &camera_modes);

	// 打印camera所支持的所有工作模式
	printf("Camera:'%s' modes are:\n", dev_name);
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



    // extern int csi_camera_set_pp_path_param(csi_cam_handle_t cam_handle, uint16_t line_num,uint16_t buf_mode);
    // csi_camera_set_pp_path_param(cam_handle,80,0);
	// 查询输出channel
	switch (channel_id) {
		case 0:
			chn_id = CSI_CAMERA_CHANNEL_0;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL0;
			break;
		case 1:
			chn_id = CSI_CAMERA_CHANNEL_1;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL1;
			break;
		case 2:
			chn_id = CSI_CAMERA_CHANNEL_2;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL2;
			break;
		case 3:
			chn_id = CSI_CAMERA_CHANNEL_3;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL3;
			break;
		case 4:
			chn_id = CSI_CAMERA_CHANNEL_4;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL4;
			break;
		case 5:
			chn_id = CSI_CAMERA_CHANNEL_5;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL5;
			break;
		case 6:
			chn_id = CSI_CAMERA_CHANNEL_6;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL6;
			break;
		case 7:
			chn_id = CSI_CAMERA_CHANNEL_7;
			env_type = CSI_CAMERA_EVENT_TYPE_CHANNEL7;
			break;
		default:
			LOG_E("fail to check chn_id = %d unsupport\n", chn_id);
			exit(-1);
	}
	csi_camera_channel_cfg_s chn_cfg;
	chn_cfg.chn_id = chn_id;
	csi_camera_channel_query(cam_handle, &chn_cfg);
	if (chn_cfg.status != CSI_CAMERA_CHANNEL_CLOSED) {
		LOG_E("Can't open chn_id = %d\n", chn_id);
		exit(-1);
	}

    extern int csi_camera_set_pp_path_param(csi_cam_handle_t cam_handle, uint16_t line_num,uint16_t buf_mode);
    /*set PP line num to default and enable sram*/
    csi_camera_set_pp_path_param(cam_handle,0,1);
	// 打开输出channel
	chn_cfg.chn_id = chn_id;
	chn_cfg.frm_cnt = 1;
	chn_cfg.img_fmt.width = width;
	chn_cfg.img_fmt.height = height;

	chn_cfg.img_fmt.pix_fmt = fmt;
	chn_cfg.img_type = CSI_IMG_TYPE_DMA_BUF;
	chn_cfg.meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
	chn_cfg.capture_type = CSI_CAMERA_CHANNEL_CAPTURE_VIDEO |
			       CSI_CAMERA_CHANNEL_CAPTURE_META;
	ret = csi_camera_channel_open(cam_handle, &chn_cfg);
	if (ret) {
		exit(-1);
	}
	get_system_time(__func__, __LINE__);
	/*init for vi->vo plink*/
      void * display_plink = NULL;
	if (display_enable == 1) {
		chn_cfg.chn_id = chn_id;
		display_plink = vi_plink_create(&chn_cfg);
        if(display_plink==NULL)
        {
            LOG_E("plink create fail\n");
            exit(-1);
        }
    }
	// 订阅Event
	csi_cam_event_handle_t event_handle;
	csi_camera_create_event(&event_handle, cam_handle);
	csi_camera_event_subscription_s subscribe;
	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CAMERA;      // 订阅Camera的ERROR事件
	subscribe.id = CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
	csi_camera_subscribe_event(event_handle, &subscribe);
	subscribe.type =
		env_type;    // 订阅Channel0的FRAME_READY事件
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe);

/*  for dsp algo setting
    cb_context[0].id=0;
    cb_context[0].cam_handle = cam_handle;
    cb_context[0].dsp_id = 1;
    cb_context[0].dsp_path = 2;
    char test_set[16]="init";
    cb_context[0].buf_size = 1920*1080;
    cb_context[0].ref_buf=NULL;
    csi_camera_dsp_algo_param_t algo_param_1={
        .algo_name = "dsp1_dummy_algo_flo",
        .algo_cb.cb = dsp_algo_result_cb,
        .algo_cb.context = &cb_context[0],
        .algo_cb.arg_size = sizeof(cb_args_t),
        .sett_ptr = test_set,
        .sett_size =16,
        .extra_buf_num =1,
        .extra_buf_sizes = &cb_context[0].buf_size,
        .extra_bufs = &cb_context[0].ref_buf,
    };
    
    if(csi_camera_set_dsp_algo_param(cam_handle,cb_context[0].dsp_id,cb_context[0].dsp_path, &algo_param_1))
    {
        LOG_E("set DSP algo fail\n");
        
    }
*/
	// 开始从channel中取出准备好的frame
	csi_camera_channel_start(cam_handle, chn_id);
	get_system_time(__func__, __LINE__);
	// 处理订阅的Event
	csi_frame_s frame;
	struct csi_camera_event event;

	while (running) {
		if (frame_num != 0) {
			if (demo_fps.frameNumber > frame_num) {
				running = false;
			}
		}
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
		csi_camera_get_event(event_handle, &event, timeout);
		// printf("%s event.type = %d, event.id = %d\n", __func__, event.type, event.id);
		switch (event.type) {
		case CSI_CAMERA_EVENT_TYPE_CAMERA:
			switch (event.id) {
			case CSI_CAMERA_EVENT_ERROR:
				// do sth.
				    LOG_O("get CAMERA EVENT CSI_CAMERA_EVENT_ERROR!\n");
				break;
            case CSI_CAMERA_EVENT_WARNING:
            		LOG_O("get CAMERA EVENT CSI_CAMERA_EVENT_WRN,RC: %s\n",event.bin);
				    break;
			default:
				break;
			}
			break;
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL2:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL3:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL4:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL5:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL6:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL7:
			switch (event.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
				get_system_time(__func__, __LINE__);
				int read_frame_count = csi_camera_get_frame_count(cam_handle,
						       chn_id);
				unsigned  long diff;
				if (init_time.tv_usec == 0)
					gettimeofday(&init_time,0);//osGetTick();
				gettimeofday(&cur_time, 0);
				diff = 1000000 * (cur_time.tv_sec-init_time.tv_sec)+ cur_time.tv_usec-init_time.tv_usec;
				demo_fps.frameNumber++;
				if (diff != 0)
					demo_fps.fps = (float)demo_fps.frameNumber / diff * 1000000.0f;
				LOG_O("%s %d read_frame_count = %d, frame_count = %ld, fps = %.2f diff = %ld\n", __func__, __LINE__, read_frame_count, demo_fps.frameNumber, demo_fps.fps, diff);

				for (int i = 0; i < read_frame_count; i++) {
					csi_camera_get_frame(cam_handle, chn_id, &frame, timeout);
#ifdef PLATFORM_SIMULATOR
                    if (frame.img.type == CSI_IMG_TYPE_DMA_BUF) {
                        void *phyaddr = vi_mem_import(frame.img.dmabuf[0].fds);
                        void *p = vi_mem_map(phyaddr) + frame.img.dmabuf[0].offset;
					    show_frame_image(p, frame.img.height, frame.img.width);
                        vi_mem_release(phyaddr);
                    } else {
					    show_frame_image(frame.img.usr_addr[0], frame.img.height, frame.img.width);
                    }
#endif
					// printf("main 443 phyaddr=%p\n", vi_mem_import(frame.img.dmabuf[0].fds));
					dump_camera_meta(&frame);
					chn_cfg.chn_id = chn_id;
					csi_camera_channel_query(cam_handle, &chn_cfg);
					frame.img.pix_format = chn_cfg.img_fmt.pix_fmt;

					if (dump_enable) {
						dump_camera_frame(&frame, chn_cfg.img_fmt.pix_fmt);
					}
					if (display_enable == 1) {
                        display_camera_frame(display_plink, &frame);
                    }
                    // csi_camera_frame_unlock(cam_handle, &frame);
                    csi_camera_put_frame(&frame);

					csi_frame_release(&frame);
				}
				break;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	csi_camera_channel_stop(cam_handle, chn_id);
	usleep (1000000);
	// 取消订阅某一个event, 也可以直接调用csi_camera_destory_event，结束所有的订阅
	subscribe.type = env_type;
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	csi_camera_destory_event(event_handle);

	csi_camera_channel_close(cam_handle, chn_id);
	csi_camera_close(cam_handle);
    if(display_plink)
    {
        vi_plink_release(display_plink);
    }
}

static void dump_camera_meta(csi_frame_s *frame)
{
	int i;

	if (frame->meta.type != CSI_META_TYPE_CAMERA)
		return;

	csi_camera_meta_s *meta_data = (csi_camera_meta_s *)frame->meta.data;
	int meta_count = meta_data->count;
	csi_camrea_meta_unit_s meta_unit;


	csi_camera_frame_get_meta_unit(
		&meta_unit, meta_data, CSI_CAMERA_META_ID_FRAME_ID);
	LOG_O("meta_id=%d, meta_type=%d, meta_value=%d\n",
			meta_unit.id, meta_unit.type, meta_unit.int_value);
}

static void dump_camera_frame(csi_frame_s *frame, csi_pixel_fmt_e fmt)
{
	char file[128];
	static int file_indx=0;
	int size;
	uint32_t indexd, j;
    void *buf[3];
    int buf_size;
    csi_camera_meta_s *meta_data = (csi_camera_meta_s *)frame->meta.data;
	csi_camrea_meta_unit_s meta_unit;


	csi_camera_frame_get_meta_unit(
		&meta_unit, meta_data, CSI_CAMERA_META_ID_CAMERA_NAME);

              
	sprintf(file,"demo_save_img%s_%d_%d",meta_unit.str_value,file_id, file_indx++%10);
	int fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH);
	if(fd == -1) {
		LOG_E("%s, %d, open file error!!!!!!!!!!!!!!!\n", __func__, __LINE__);
		return ;
	}
    if (frame->img.type == CSI_IMG_TYPE_DMA_BUF) {

        buf_size = frame->img.strides[0]*frame->img.height;
        switch(fmt) {
                case CSI_PIX_FMT_NV12:
		        case CSI_PIX_FMT_YUV_SEMIPLANAR_420:
                        if(frame->img.num_planes ==2)
                        { 
                            buf_size+=frame->img.strides[1]*frame->img.height/2;
                        }
                        else{
                            LOG_E("img fomrat is not match with frame planes:%d\n",frame->img.num_planes);
                            return;
                        }
                        break;
                case CSI_PIX_FMT_YUV_SEMIPLANAR_422:
                        if(frame->img.num_planes ==2)
                        { 
                            buf_size+=frame->img.strides[1]*frame->img.height;
                        }
                        else{
                            LOG_E("img fomrat is not match with frame planes:%d\n",frame->img.num_planes);
                            return;
                        }
                        break;
                default:
                        break;
                
        }

        buf[0]= mmap(0, buf_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, frame->img.dmabuf[0].fds, frame->img.dmabuf[0].offset);
        // plane_size[1] = frame->img.strides[1]*frame->img.height;
        // printf("frame plane 1 stride:%d,offset:%d\n", frame->img.strides[1],(uint32_t)frame->img.dmabuf[1].offset);
        if(frame->img.num_planes ==2)
        {
            // buf[1] = mmap(0, plane_size[1]/2, PROT_READ | PROT_WRITE,
            //             MAP_SHARED, frame->img.dmabuf[1].fds, frame->img.dmabuf[1].offset);
            buf[1] = buf[0]+frame->img.dmabuf[1].offset;
        }

    }
    else{
            buf[0] = frame->img.usr_addr[0];
            buf[1] = frame->img.usr_addr[1];
    }

	LOG_O("%s,save img from : (%lx,%lx)size:%d to %s, fmt:%d width:%d stride:%d height:%d\n",__FUNCTION__, (uint64_t)buf[0],(uint64_t)buf[1],size, file, fmt, frame->img.width, frame->img.strides[0], frame->img.height);

	if (frame->img.strides[0] == 0) {
		frame->img.strides[0] = frame->img.width;
	}
	switch(fmt) {
		case CSI_PIX_FMT_NV12:
		case CSI_PIX_FMT_YUV_SEMIPLANAR_420:
			for (j = 0; j < frame->img.height; j++) { //Y
				indexd = j*frame->img.strides[0];
				write(fd, buf[0] + indexd, frame->img.width);
			}
			for (j = 0; j < frame->img.height / 2; j++) { //UV
				indexd = j*frame->img.strides[0];
				write(fd, buf[1] + indexd, frame->img.width);
			}
			break;
		case CSI_PIX_FMT_RGB_INTEVLEAVED_888:
		case CSI_PIX_FMT_YUV_TEVLEAVED_444:
            size = frame->img.width*3;
            for (j = 0; j < frame->img.height; j++) {
				indexd = j*frame->img.strides[0];
				write(fd, buf[0] + indexd, size);
			}
			break;
		case CSI_PIX_FMT_BGR:
		case CSI_PIX_FMT_RGB_PLANAR_888:
		case CSI_PIX_FMT_YUV_PLANAR_444:
			size = frame->img.width * frame->img.height * 3;
			write(fd, buf[0], size);
			break;
        case CSI_PIX_FMT_RAW_8BIT:
            size = frame->img.width;
            for (j = 0; j < frame->img.height; j++) {
				indexd = j*frame->img.strides[0];
				write(fd, buf[0] + indexd, size);
			}
			break;
		case CSI_PIX_FMT_YUV_TEVLEAVED_422:
        case CSI_PIX_FMT_RAW_10BIT:
	    case CSI_PIX_FMT_RAW_12BIT:
	    case CSI_PIX_FMT_RAW_14BIT:
	    case CSI_PIX_FMT_RAW_16BIT:
            size = frame->img.width*2;
            for (j = 0; j < frame->img.height; j++) {
				indexd = j*frame->img.strides[0];
				write(fd, buf[0] + indexd, size);
			}
			break;
		case CSI_PIX_FMT_YUV_SEMIPLANAR_422:
			if (frame->img.strides[0] == 0) {
				frame->img.strides[0] = frame->img.width;
			}
			for (j = 0; j < frame->img.height; j++) { //Y
				indexd = j*frame->img.strides[0];
				write(fd,buf[0] + indexd, frame->img.width);
			}
			for (j = 0; j < frame->img.height; j++) { //UV
				indexd = j*frame->img.strides[0];
				write(fd, buf[1]+ indexd, frame->img.width);
			}
			break;
		default:
			LOG_E("%s unsupported format to save\n", __func__);
			exit(-1);
			break;
	}

	close(fd);
    munmap(buf[0],buf_size);
    // munmap(buf[1],plane_size[1]);
	LOG_O("%s exit\n", __func__);
}
