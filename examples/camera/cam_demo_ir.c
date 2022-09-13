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
#include <fcntl.h>
#include <unistd.h>
#include "pthread.h"
#include <sys/mman.h>
#define LOG_LEVEL 2
#define LOG_PREFIX "cam_demo_ir"
#include <syslog.h>


#include <csi_frame.h>
#include <csi_camera.h>
#include "csi_camera_dev_api.h"
#define TEST_DEVICE_NAME "/dev/video12"
typedef struct frame_fps {
	uint64_t frameNumber;
	struct timeval initTick;
	float fps;
} frame_fps_t;
static void dump_camera_meta(csi_frame_s *frame);
static void dump_camera_frame(csi_frame_s *frame, int ch_id);


typedef struct event_queue_item{
    struct csi_camera_event evet;
    struct event_queue_item *next;
}event_queue_item_t;

typedef struct channel_handle_ctx{

        struct event_queue{
         event_queue_item_t *head;
         event_queue_item_t *tail;
         int exit;
        pthread_mutex_t mutex;
        pthread_cond_t cond;
    }event_queue;
    csi_cam_handle_t cam_handle;
    pthread_t  chan_thread;
    int frame_num;
}channel_handle_ctx_t;

static  event_queue_item_t *dequeue_event(channel_handle_ctx_t *ctx)
{
	event_queue_item_t *ev = ctx->event_queue.head;
    pthread_mutex_lock(&ctx->event_queue.mutex);
	if(!ev)
    {
        pthread_mutex_unlock(&ctx->event_queue.mutex);
        return NULL;
    }

	if(ev == ctx->event_queue.tail)
		ctx->event_queue.tail = NULL;
	ctx->event_queue.head = ev->next;
    pthread_mutex_unlock(&ctx->event_queue.mutex);
    return ev;
}

static int test_flag =0;
static void enqueue_event(channel_handle_ctx_t *ctx,event_queue_item_t * item)
{

	if(!item || !ctx)
		return;
    item->next = NULL;
	LOG_D("%s enter \n", __func__);
	pthread_mutex_lock(&ctx->event_queue.mutex);
	if (ctx->event_queue.tail) {
		ctx->event_queue.tail->next = item;
	} else {
		ctx->event_queue.head = item;
        // pthread_cond_broadcast(&ctx->cond.cond);
	}
	ctx->event_queue.tail = item;
	pthread_mutex_unlock(&ctx->event_queue.mutex);
}
static void* camera_channle_process(void* arg)
{

	struct timeval init_time, cur_time;
    frame_fps_t demo_fps;
    csi_frame_s frame;
	memset(&init_time, 0, sizeof(init_time));
	memset(&cur_time, 0, sizeof(cur_time));
    channel_handle_ctx_t * ctx = (channel_handle_ctx_t *)arg;
    int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
    LOG_D("Channel process running........\n");
    while(ctx->event_queue.exit == 0)
    {
        event_queue_item_t *item = dequeue_event(ctx);
        if(item==NULL)
        {
            continue;
        }

	switch (item->evet.type) {
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
			switch (item->evet.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
                csi_camera_channel_id_e ch_id = item->evet.type-CSI_CAMERA_EVENT_TYPE_CHANNEL0+CSI_CAMERA_CHANNEL_0;
				int read_frame_count = csi_camera_get_frame_count(ctx->cam_handle,
						       ch_id);
				for (int i = 0; i < read_frame_count; i++) {
					csi_camera_get_frame(ctx->cam_handle, ch_id, &frame, timeout);
                    demo_fps.frameNumber++;
                    ctx->frame_num++;
                    dump_camera_meta(&frame);
                    if(test_flag ==0)
                    {
                        dump_camera_frame(&frame,ch_id);
                    }

                    // csi_camera_frame_unlock(cam_handle, &frame);
                    csi_camera_put_frame(&frame);
					csi_frame_release(&frame);
				}
                unsigned  long diff;
				if (init_time.tv_usec == 0)
					gettimeofday(&init_time,0);//osGetTick();
				gettimeofday(&cur_time, 0);
				diff = 1000000 * (cur_time.tv_sec-init_time.tv_sec)+ cur_time.tv_usec-init_time.tv_usec;
		
				if (diff != 0)
					demo_fps.fps = (float)demo_fps.frameNumber / diff * 1000000.0f;
				LOG_O("ch id:%d read_frame_count = %d, frame_count = %d, fps = %.2f diff = %ld\n",ch_id, read_frame_count, ctx->frame_num, demo_fps.fps, diff);

				break;
			}
			default:
				break;
			}
		default:
			break;
		}
        free(item);
    }
    LOG_O("exit\n");
    pthread_exit(0);
}
channel_handle_ctx_t  channel_ctx[2];
/***********************DSP ALGO *******************************/

typedef struct cb_context{
    int id;
    csi_cam_handle_t cam_handle;
    int dsp_id;
    int dsp_path;
    void *ref_buf;
    int buf_size;
}cb_context_t;

typedef struct cb_args{
    char lib_name[8];
    char setting[16];
    uint32_t frame_id;
    uint64_t timestap;
    uint32_t  temp_projector;   //投射器温度
	uint32_t  temp_sensor;     //sensor 温度
}cb_args_t;
cb_context_t cb_context[2];
void dsp_algo_result_cb(void*context,void*arg)
{
   cb_context_t *ctx = (cb_context_t *)context;
    cb_args_t *cb_args= (cb_args_t*)arg;    
    char update_setting[16];
    struct timeval times_value;
    uint32_t frame_id = cb_args->frame_id;
    uint64_t frame_timestap =  cb_args->timestap;
    times_value.tv_sec = cb_args->timestap/1000000;
    times_value.tv_usec = cb_args->timestap%1000000;

    LOG_O("cb:%d,%s,setting:%s,frame :%d,timestad:(%ld s,%ld us)\n",ctx->id,cb_args->lib_name,cb_args->setting,
                                                            frame_id,times_value.tv_sec,times_value.tv_usec);
    LOG_O("temperate projector:%d,sensor:%d\n",cb_args->temp_projector,cb_args->temp_sensor);

    if(test_flag!=0)
    {
        sprintf(update_setting, "update_%d", frame_id);
        csi_camera_update_dsp_algo_setting(ctx->cam_handle,ctx->dsp_id,ctx->dsp_path,update_setting);
        if(frame_id%20==0)
        {
            void *replace_buf;
            *((uint32_t*)ctx->ref_buf) = frame_id;
            csi_camera_update_dsp_algo_buf(ctx->cam_handle, ctx->dsp_id,ctx->dsp_path,ctx->ref_buf,&replace_buf);
            LOG_D("cb:%d,old buffer:0x%llx,new buffer:0x%llx\n",ctx->id,ctx->ref_buf,replace_buf);
            ctx->ref_buf = replace_buf;
        }
    }
}
static void usage(void)
{
	printf("    1 : video id  cases\n");
    printf("    2 : [0]dump frame or [1] algo loop test\n");
    printf("    3 : frame num to stop ,0: for not stop\n");
	printf("    4 : channel 0 algo\n");
	printf("    5 : channel 1 algo\n");

}

int main(int argc, char *argv[])
{
	uint32_t i;
	bool running = true;
	csi_camera_info_s camera_info;
	frame_fps_t demo_fps;
    char *algo_1;
    char *algo_2;
    int video_id = -1;
    char dev_name[64];
    int frame_num = 0;
    algo_2 = "dsp1_dummy_algo_flo";
    algo_1 = "dsp1_dummy_algo_flo_1";
	switch(argc)
    {
        case 0:
        case 1:
                 LOG_E("input ERROR:vidoe id is mandatory\n");
                 usage();
                 return -1;
        
        case 6:
                algo_2 = (argv[5]);
        case 5:
                algo_1 = (argv[4]);
        case 4:
                frame_num = atoi(argv[3]);
        case 3:
                test_flag = atoi(argv[2]);
        case 2:
                video_id = atoi(argv[1]);
                break;
        default:
            LOG_E("input ERROR:too many param\n");
            usage();
            return -1;
    }
    LOG_O("Demo: video id:%d,test type:%d,frame number:%d,algo1:%s,algo2:%s\n",video_id,test_flag,frame_num,algo_1,algo_2);
    sprintf(dev_name, "/dev/video%d", video_id);
	memset(&demo_fps, 0, sizeof(demo_fps));
	struct timeval init_time, cur_time;
	memset(&init_time, 0, sizeof(init_time));
	memset(&cur_time, 0, sizeof(cur_time));
	// 获取设备中，所有的Camera
	struct csi_camera_infos camera_infos;
	memset(&camera_infos, 0, sizeof(camera_infos));
	csi_camera_query_list(&camera_infos);
	LOG_O("csi_camera_query_list() OK\n");

	// 打印所有设备所支持的Camera
	for (i = 0; i < camera_infos.count; i++) {
		printf("Camera[%d]: camera_name='%s', device_name='%s', bus_info='%s', capabilities=0x%08x\n",
		       i, camera_infos.info[i].camera_name, camera_infos.info[i].device_name,
		       camera_infos.info[i].bus_info, camera_infos.info[i].capabilities);
	}
	/*  Camera[0]: camera_name='RGB_Camera', device_name='/dev/video0', bus_info='CSI-MIPI', capabilities=0x00800001
	 *  Camera[1]: camera_name:'Mono_Camera', device_name:'/dev/video8', bus_info='USB', capabilities=0x00000001
	 */

	bool found_camera = false;
	for (i = 0; i < camera_infos.count; i++) {
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

	printf("The caps are:\n");
	for (i = 1; i < 0x80000000; i = i << 1) {
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
	printf(" Camera:'%s' modes are:{\n", dev_name);
	for (i = 0; i < camera_modes.count; i++) {
		printf("\t mode_id=%d: description:'%s'\n",
		       camera_modes.modes[i].mode_id, camera_modes.modes[i].description);
	}
	printf("}\n");

	
	csi_camera_channel_cfg_s chn_cfg;

	chn_cfg.chn_id = CSI_CAMERA_CHANNEL_0;
	chn_cfg.frm_cnt = 4;
	chn_cfg.img_fmt.width = 1080;
	chn_cfg.img_fmt.height = 1280;
	chn_cfg.img_fmt.pix_fmt = CSI_PIX_FMT_RAW_12BIT;
	chn_cfg.img_type = CSI_IMG_TYPE_DMA_BUF;
	chn_cfg.meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
	chn_cfg.capture_type = CSI_CAMERA_CHANNEL_CAPTURE_VIDEO |
			       CSI_CAMERA_CHANNEL_CAPTURE_META;
	csi_camera_channel_open(cam_handle, &chn_cfg);
	LOG_O("CSI_CAMERA_CHANNEL_0: csi_camera_channel_open() OK\n");


	// 打开输出channel_1
	chn_cfg.chn_id = CSI_CAMERA_CHANNEL_1;
	chn_cfg.frm_cnt = 4;
	chn_cfg.img_fmt.width = 1080;
	chn_cfg.img_fmt.height = 1280;
	chn_cfg.img_fmt.pix_fmt = CSI_PIX_FMT_RAW_12BIT;
	chn_cfg.img_type = CSI_IMG_TYPE_DMA_BUF;
	chn_cfg.meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
	chn_cfg.capture_type = CSI_CAMERA_CHANNEL_CAPTURE_VIDEO |
			       CSI_CAMERA_CHANNEL_CAPTURE_META;
	csi_camera_channel_open(cam_handle, &chn_cfg);
	LOG_O("CSI_CAMERA_CHANNEL_1: csi_camera_channel_open() OK\n");

	// 订阅Event
	csi_cam_event_handle_t event_handle;
	csi_camera_create_event(&event_handle, cam_handle);
	struct csi_camera_event_subscription subscribe;
	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CAMERA;      // 订阅Camera的ERROR事件
	subscribe.id = CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
	csi_camera_subscribe_event(event_handle, &subscribe);

	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CHANNEL0;    // 订阅Channel0的FRAME_READY事件
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe);
    memset(&channel_ctx[0],0x0,sizeof(channel_handle_ctx_t));
    channel_ctx[0].cam_handle = cam_handle;
    pthread_mutex_init(&channel_ctx[0].event_queue.mutex,NULL);
    pthread_cond_init(&channel_ctx[0].event_queue.cond,NULL);
   pthread_create(&channel_ctx[0].chan_thread,NULL,camera_channle_process,&channel_ctx[0]);

	subscribe.type =
		CSI_CAMERA_EVENT_TYPE_CHANNEL1;    // 订阅Channel0的FRAME_READY事件
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;
	csi_camera_subscribe_event(event_handle, &subscribe);
    memset(&channel_ctx[1],0x0,sizeof(channel_handle_ctx_t));
    channel_ctx[1].cam_handle = cam_handle;
    pthread_mutex_init(&channel_ctx[1].event_queue.mutex,NULL);
    pthread_cond_init(&channel_ctx[1].event_queue.cond,NULL);
    pthread_create(&channel_ctx[1].chan_thread,NULL,camera_channle_process,&channel_ctx[1]);
	LOG_O("Event subscript OK\n");
    
    cb_context[0].id=0;
    cb_context[0].cam_handle = cam_handle;
    cb_context[0].dsp_id = 1;
    cb_context[0].dsp_path = 3;    
    char test_set[16]="init";
    cb_context[0].buf_size = 1920*1080;
    cb_context[0].ref_buf=NULL;
    csi_camera_dsp_algo_param_t algo_param_1={
        .algo_name = algo_1,
        .algo_cb.cb = dsp_algo_result_cb,
        .algo_cb.context = &cb_context[0],
        .algo_cb.arg_size =  sizeof(cb_args_t),
        .sett_ptr = test_set,
        .sett_size =sizeof(test_set),
        .extra_buf_num =1,
        .extra_buf_sizes = &cb_context[0].buf_size,
        .extra_bufs = &cb_context[0].ref_buf,
    };
    
    if(csi_camera_set_dsp_algo_param(cam_handle, cb_context[0].dsp_id,cb_context[0].dsp_path, &algo_param_1))
    {
        LOG_E("set DSP algo fail\n");
        
    }


    cb_context[1].id=1;
    cb_context[1].cam_handle = cam_handle;
    cb_context[1].dsp_id = 1;
    cb_context[1].dsp_path = 4;    
    cb_context[1].buf_size = 1920*1080;
    cb_context[1].ref_buf=NULL;
    csi_camera_dsp_algo_param_t algo_param_2={
        .algo_name = algo_2,
        .algo_cb.cb = dsp_algo_result_cb,
        .algo_cb.context = &cb_context[1],
        .algo_cb.arg_size = sizeof(cb_args_t),
        .sett_ptr = test_set,
        .sett_size =sizeof(test_set),
        .extra_buf_num =1,
        .extra_buf_sizes = &cb_context[1].buf_size,
        .extra_bufs = &cb_context[1].ref_buf,
    };
    
    if(csi_camera_set_dsp_algo_param(cam_handle, cb_context[1].dsp_id,cb_context[1].dsp_path, &algo_param_2))
    {
        LOG_E("set DSP algo fail\n");
        
    }

    csi_camera_floodlight_led_set_flash_bright(cam_handle, 500); //500ma
    csi_camera_projection_led_set_flash_bright(cam_handle, 500); //500ma
    csi_camera_projection_led_set_mode(cam_handle, LED_IR_ENABLE);
    csi_camera_floodlight_led_set_mode(cam_handle, LED_IR_ENABLE);
    csi_camera_led_enable(cam_handle, LED_FLOODLIGHT_PROJECTION);

    // 开始从channel中取出准备好的frame
	csi_camera_channel_start(cam_handle, CSI_CAMERA_CHANNEL_0);
	csi_camera_channel_start(cam_handle, CSI_CAMERA_CHANNEL_1);
	LOG_O("Channel start OK\n");

	// 处理订阅的Event
	csi_frame_s frame;
	event_queue_item_t *event_item;

	LOG_O("Starting get frame...\n");
	while (running) {
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
        event_item = malloc(sizeof( event_queue_item_t));   
		csi_camera_get_event(event_handle, &event_item->evet, timeout);
       LOG_O("event.type = %d, event.id = %d\n",event_item->evet.type, event_item->evet.id);

		switch (event_item->evet.type) {
		case CSI_CAMERA_EVENT_TYPE_CAMERA:
			switch (event_item->evet.id) {
			case CSI_CAMERA_EVENT_ERROR:
				// do sth.
				break;
			default:
				break;
            free(event_item);
            break;
			}

		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
			switch (event_item->evet.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
                csi_camera_channel_id_e ch_id = event_item->evet.type-CSI_CAMERA_EVENT_TYPE_CHANNEL0+CSI_CAMERA_CHANNEL_0;
				 
                 if(event_item && ch_id<2)
                 {
                     enqueue_event(&channel_ctx[ch_id],event_item);
                 }
				break;
			}
			default:
                free(event_item);
				break;
			}
            break;
		default:
            free(event_item);
			break;
		}

    //    LOG_O("stop:%d,frame:(%d,%d)(%llx,%llx)\n",frame_num ,channel_ctx[0].frame_num ,channel_ctx[1].frame_num,&(channel_ctx[0].frame_num),&(channel_ctx[1].frame_num));

        if((frame_num >0) &&
            (channel_ctx[0].frame_num >frame_num) &&
            (channel_ctx[1].frame_num >frame_num))
        {
                csi_camera_channel_stop(cam_handle, CSI_CAMERA_CHANNEL_0);
                csi_camera_channel_stop(cam_handle, CSI_CAMERA_CHANNEL_1);
                channel_ctx[0].event_queue.exit = 1;
                channel_ctx[1].event_queue.exit = 1;
                pthread_join(channel_ctx[0].chan_thread,NULL);
                pthread_join(channel_ctx[1].chan_thread,NULL);
                running = false;
        }
	}

	// 取消订阅某一个event, 也可以直接调用csi_camera_destory_event，结束所有的订阅
	subscribe.type = CSI_CAMERA_EVENT_TYPE_CHANNEL0;
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	subscribe.type = CSI_CAMERA_EVENT_TYPE_CHANNEL1;
	subscribe.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY |
		       CSI_CAMERA_CHANNEL_EVENT_OVERFLOW;;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	subscribe.type = CSI_CAMERA_EVENT_TYPE_CAMERA;
	subscribe.id = CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
	csi_camera_unsubscribe_event(event_handle, &subscribe);

	csi_camera_destory_event(event_handle);

	csi_camera_channel_close(cam_handle, CSI_CAMERA_CHANNEL_0);
	csi_camera_close(cam_handle);
}

static void dump_camera_meta(csi_frame_s *frame)
{
	int i;
	if (frame->meta.type != CSI_META_TYPE_CAMERA)
		return;

	csi_camera_meta_s *meta_data = (csi_camera_meta_s *)frame->meta.data;
	csi_camrea_meta_unit_s meta_frame_id,meta_frame_ts;


	csi_camera_frame_get_meta_unit(
		&meta_frame_id, meta_data, CSI_CAMERA_META_ID_FRAME_ID);
    csi_camera_frame_get_meta_unit(
		&meta_frame_ts, meta_data, CSI_CAMERA_META_ID_TIMESTAMP);
	LOG_O("meta frame id=%d,time stap:(%ld,%ld)\n",
			 meta_frame_id.int_value,meta_frame_ts.time_value.tv_sec,meta_frame_ts.time_value.tv_usec);
}

static void dump_camera_frame(csi_frame_s *frame,  int ch_id)
{
	char file[128];
	static int file_indx=0;
	int size,buf_size;
	uint32_t indexd, j;
    void *buf[3];

	sprintf(file,"demo_save_img_ch_%d_%d", ch_id,file_indx++%6);
	int fd = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH);
	if(fd == -1) {
		LOG_E("%s, %d, open file error!!!!!!!!!!!!!!!\n", __func__, __LINE__);
		return ;
	}
    if (frame->img.type == CSI_IMG_TYPE_DMA_BUF) {

        buf_size = frame->img.strides[0]*frame->img.height;
        switch(frame->img.pix_format) {
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

	LOG_O("save img from :%d, to %s, fmt:%d width:%d stride:%d height:%d\n",frame->img.dmabuf[0].fds, file, frame->img.pix_format , frame->img.width, frame->img.strides[0], frame->img.height);
    frame->img.pix_format = CSI_PIX_FMT_RAW_12BIT;
	switch(frame->img.pix_format) {
		case CSI_PIX_FMT_NV12:
		case CSI_PIX_FMT_YUV_SEMIPLANAR_420:
            if (frame->img.strides[0] == 0) {
				frame->img.strides[0] = frame->img.width;
			}
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