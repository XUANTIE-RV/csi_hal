/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: ShenWuYi <shenwuyi.swy@alibaba-inc.com>
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
#include <sys/mman.h>
#define LOG_LEVEL 2
#define LOG_PREFIX "cam_demo_multi"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>
#include <csi_camera_dev_api.h>
#ifdef PLATFORM_SIMULATOR
#include "apputilities.h"
#endif
#define  MAX_CAM_NUM    3
#define  MAX_CHANNEL_NUM 3 
static void dump_camera_meta(csi_frame_s *frame);
static void dump_camera_frame(csi_frame_s *frame, csi_pixel_fmt_e fmt, int camera_id);

// extern int csi_camera_frame_unlock(csi_cam_handle_t cam_handle, csi_frame_s *frame);

#define TEST_DEVICE_NAME "/dev/video0"
#define CSI_CAMERA_TRUE  1
#define CSI_CAMERA_FALSE 0

typedef struct frame_fps {
	uint64_t frameNumber;
	struct timeval initTick;
	float fps;
} frame_fps_t;
typedef enum _frame_mode{
    FRAME_NONE =0,
    FRAME_SAVE_IMG =1,
    FRAME_SAVE_STREAM =2,
    FRAME_SAVE_DISPLAY,
    FRAME_INVALID,
}frame_mode_t;

typedef enum _cam_type{
    CAM_TYEP_RGB =0,
    CAM_TYEP_IR,
    CAM_TYEP_INVALID,

}cam_type_e;


typedef struct _camera_param{
    int video_id;
    int channel_num;
    struct {
        int width;
        int height;
        csi_pixel_fmt_e fmt;

    }out_pic[MAX_CHANNEL_NUM];

    cam_type_e  type;
    frame_mode_t  mode;
    int frames_to_stop;
}camera_param_t;

typedef struct event_queue_item{
    struct csi_camera_event evet;
    struct event_queue_item *next;
}event_queue_item_t;

typedef struct _camera_ctx{

    int cam_id;
    pthread_t  cam_thread;
    int exit;
    csi_cam_handle_t cam_handle;
    int  channel_num;
    csi_cam_event_handle_t event_handle;
    csi_camera_event_subscription_s event_subscribe;
 	csi_camera_channel_cfg_s chn_cfg[MAX_CHANNEL_NUM];
    void * disaply_plink[MAX_CHANNEL_NUM];
    int frame_num;
    frame_mode_t  frame_mode[MAX_CHANNEL_NUM];
}camera_ctx_t;

int file_id = 0;
extern void *vi_plink_create(csi_camera_channel_cfg_s *chn_cfg);
extern void vi_plink_release(void * plink);
extern void display_camera_frame(void * plink, csi_frame_s *frame);

static void usage(void)
{
	printf("    1 : RGB Camera id \n");
	printf("    2 : RGB Camera id channel num\n");
	printf("    3 : RGB Camera frame mode :0 none ,1 dump enable,2 display \n");
    printf("    4 : RGB Camera frame num :0 not stop ,else frame num to stop \n");
	printf("    5 : IR Camera id \n");
	printf("    6 : IR Camera id channel num\n");
	printf("    7 : IR Camera frame mode :0 none ,1 dump enable,2 display \n");
    printf("    8 : IR Camera frame num :0 not stop ,else frame num to stop \n");
}

void printUsage(char *name)
{
    printf("usage: %s [options]\n"
           "\n"
           "  Available options:\n"
           "    -f      fisrt camera device 5 param \n"
           "            1:video id 2: channel num  3: camrea type (0->rgb ;1->IR)  (mandatory)\n"
           "            3: mode【0->none ,1->save image,2->display 】(default 1) \n"
           "            4: frame num to stop 【0 not stop】(default 0) \n"
           "    -s      second camera device 5 param)\n"
           "            same param setting as above)\n"
           "    -t      third camera device 5 param)\n"
           "           same param setting  above)\n"

           "\n", name);
}

static int  parseParams(int argc, char **argv, camera_param_t *params)
{
        int index =0;
        params[index].video_id = 2;
        params[index].type = CAM_TYEP_RGB;

        params[index].channel_num =1;
        params[index].out_pic[0].width = 1920;
        params[index].out_pic[0].height = 1080;
        params[index].out_pic[0].fmt = CSI_PIX_FMT_NV12;
        params[index].mode = FRAME_SAVE_IMG;
        params[index].frames_to_stop = 0;
        index++;

        params[index].video_id = 6;
        params[index].type = CAM_TYEP_IR;

        params[index].channel_num = 2;
        params[index].out_pic[0].width = 1080;
        params[index].out_pic[0].height = 1280;
        params[index].out_pic[0].fmt =CSI_PIX_FMT_RAW_12BIT;
        params[index].out_pic[1].width = 1080;
        params[index].out_pic[1].height = 1280;
        params[index].out_pic[1].fmt =CSI_PIX_FMT_RAW_12BIT;

        params[index].mode = FRAME_SAVE_IMG;
        params[index].frames_to_stop = 0;
        index++;
        return index;   
}

static void get_system_time(const char *func, int line_num)
{
	struct timeval cur_time;

	memset(&cur_time, 0, sizeof(cur_time));
	gettimeofday(&cur_time, 0);
	printf("%s %s line_num = %d, cur_time.tv_sec = %ld, cur_time.tv_usec = %ld\n",
		__func__, func, line_num, cur_time.tv_sec, cur_time.tv_usec);
}


static int camera_get_chl_id(int env_type, int *chl_id)
{
	if (chl_id == NULL) {
		LOG_E("fail to check param chl_id = %p\n", chl_id);
		return -1;
	}
	switch (env_type) {
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
			*chl_id = CSI_CAMERA_CHANNEL_0;
			break;
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
			*chl_id = CSI_CAMERA_CHANNEL_1;
			break;
		case CSI_CAMERA_EVENT_TYPE_CHANNEL2:
			*chl_id = CSI_CAMERA_CHANNEL_2;
			break;
		default:
			LOG_D("%s fail to check env_type = %d unsupport\n", env_type);
			break;
	}
	return 0;
}

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

    // if(test_flag!=0)
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
static void *camera_event_thread(void *arg)
{
	int ret = 0;
	if (arg == NULL) {
		LOG_E("NULL Ptr\n");
        pthread_exit(0);
	}
	camera_ctx_t* ctx = (camera_ctx_t*)arg;
	csi_cam_event_handle_t ev_handle = ctx->event_handle;
    csi_camera_channel_cfg_s  *ch_cfg=NULL;
	struct timeval init_time, cur_time;
	memset(&init_time, 0, sizeof(init_time));
	memset(&cur_time, 0, sizeof(cur_time));

	frame_fps_t demo_fps;
	memset(&demo_fps, 0, sizeof(demo_fps));
	csi_frame_s frame;

	if (ev_handle == NULL) {
		LOG_E("fail to get ev_handle ev_handle\n");
		pthread_exit(0);
	}
	int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
	int loop_count = 0;
	struct csi_camera_event event;
    LOG_O("Theard is runing............\n");
	while (ctx->exit==0) {
		int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
		csi_camera_get_event(ev_handle, &event, timeout);
		LOG_D("Camera_%d event.type = %d, event.id = %d\n",ctx->cam_id ,event.type, event.id);
		switch (event.type) {
		case CSI_CAMERA_EVENT_TYPE_CAMERA:
			switch (event.id) {
			case CSI_CAMERA_EVENT_ERROR:
				// do sth.
				    LOG_E("get CAMERA EVENT CSI_CAMERA_EVENT_ERROR!\n");
				    break;
            case CSI_CAMERA_EVENT_WARNING:
            		LOG_W("get CAMERA EVENT CSI_CAMERA_EVENT_WRN,RC: %s\n",event.bin);
				    break;
			default:
				break;
			}
			break;
		case CSI_CAMERA_EVENT_TYPE_CHANNEL0:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL1:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL2:
		case CSI_CAMERA_EVENT_TYPE_CHANNEL3:
			switch (event.id) {
			case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY: {
				int chn_id = 0;
				ret = camera_get_chl_id(event.type, &chn_id);
				if (ret) {
					LOG_E("fail to get chl_id = %d\n", chn_id);
					continue;
				}
                ch_cfg = &ctx->chn_cfg[chn_id];
				get_system_time(__func__, __LINE__);
				int read_frame_count = csi_camera_get_frame_count(ctx->cam_handle,
						       chn_id);

				unsigned  long diff;
				if (init_time.tv_usec == 0)
					gettimeofday(&init_time,0);//osGetTick();
				gettimeofday(&cur_time, 0);
				diff = 1000000 * (cur_time.tv_sec-init_time.tv_sec)+ cur_time.tv_usec-init_time.tv_usec;
				demo_fps.frameNumber++;
				if (diff != 0)
					demo_fps.fps = (float)demo_fps.frameNumber / diff * 1000000.0f;
				LOG_O("cam:%d,channle:%d ,read_frame_count = %d, frame_count = %ld, fps = %.2f diff = %ld\n", ctx->cam_id, chn_id, read_frame_count, demo_fps.frameNumber, demo_fps.fps, diff);

				for (int i = 0; i < read_frame_count; i++) {
					csi_camera_get_frame(ctx->cam_handle, chn_id, &frame, timeout);
					dump_camera_meta(&frame);
					if (ctx->frame_mode[chn_id] == FRAME_SAVE_IMG) {
						dump_camera_frame(&frame, ch_cfg->img_fmt.pix_fmt, chn_id);
					}
					if (ctx->disaply_plink && ctx->frame_mode[chn_id]==FRAME_SAVE_DISPLAY) {
						frame.img.pix_format = ch_cfg->img_fmt.pix_fmt;
						display_camera_frame(ctx->disaply_plink, &frame);
                    }
                    csi_camera_put_frame(&frame);
                    ctx->frame_num++;
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
	LOG_O("exit!\n");
	pthread_exit(0);
}


static void camera_start_all(camera_ctx_t *  ctx)
{
    camera_ctx_t * cam_ctx = ctx;
    int loop_ch;
    for(loop_ch=0;loop_ch<cam_ctx->channel_num;loop_ch++)
    {
	    csi_camera_channel_start(cam_ctx->cam_handle, cam_ctx->chn_cfg[loop_ch].chn_id);
    }
}

static void camera_stop_all(camera_ctx_t *  ctx)
{
    camera_ctx_t * cam_ctx = ctx;
    int loop_ch;
    for(loop_ch=0;loop_ch<cam_ctx->channel_num;loop_ch++)
    {
	    csi_camera_channel_stop(cam_ctx->cam_handle, cam_ctx->chn_cfg[loop_ch].chn_id);
    }
}
static camera_ctx_t * camera_open(camera_param_t *params)
{
	int ret = 0;
	char dev_name[128];
    camera_ctx_t * cam_ctx = NULL;
    int loop_ch;
	LOG_O("Open Camera %d\n",params->video_id);
    if(params==NULL || params->video_id <0)
    {
        LOG_E("param err\n");
        return NULL;
    }
    if(params->channel_num> MAX_CHANNEL_NUM)
    {
        LOG_E("unsupoort channle num:%d \n",params->channel_num);
        return NULL;
    }

    if(params->mode >=FRAME_INVALID)
    {
        LOG_E("unsupoort frame process mode:%d \n",params->mode);
        return NULL;
    }

    cam_ctx = malloc(sizeof(camera_ctx_t));
    if(!cam_ctx)
    {
        return NULL;
    }
    memset(cam_ctx,0x0,sizeof(camera_ctx_t));

  
	// 打开Camera设备获取句柄，作为后续操对象
    sprintf(dev_name, "/dev/video%d", params->video_id);
	if(csi_camera_open(&cam_ctx->cam_handle, dev_name))
    {
        LOG_E("Fail to open cam :%s\n",dev_name);
        goto ONE_ERR; 
    }

    for(loop_ch= CSI_CAMERA_CHANNEL_0;loop_ch<params->channel_num;loop_ch++)
    {
        cam_ctx->chn_cfg[loop_ch].chn_id = loop_ch;
        cam_ctx->chn_cfg[loop_ch].img_fmt.pix_fmt = params->out_pic[loop_ch].fmt;
        cam_ctx->chn_cfg[loop_ch].img_fmt.width= params->out_pic[loop_ch].width;
        cam_ctx->chn_cfg[loop_ch].img_fmt.height = params->out_pic[loop_ch].height;
        cam_ctx->chn_cfg[loop_ch].img_type = CSI_IMG_TYPE_DMA_BUF;
        cam_ctx->chn_cfg[loop_ch].meta_fields = CSI_CAMERA_META_DEFAULT_FIELDS;
        if(csi_camera_channel_open(cam_ctx->cam_handle,&cam_ctx->chn_cfg[loop_ch]))
        {
            LOG_E("Fail to open cam %s,channel :%d\n",dev_name,loop_ch);
            goto TWO_ERR; 
        }

        if(params->mode == FRAME_SAVE_DISPLAY)
        {
            cam_ctx->disaply_plink[loop_ch] = vi_plink_create(&cam_ctx->chn_cfg[loop_ch]);
            if(!cam_ctx->disaply_plink[loop_ch]&& loop_ch== CSI_CAMERA_CHANNEL_0)  //temp only channel to display
            {
                LOG_E("Fail to create plink for cam %s,channel :%d\n",dev_name,loop_ch);
                goto TWO_ERR; 
            }
        }
        cam_ctx->frame_mode[loop_ch]=params->mode;
        cam_ctx->channel_num++;
    }

    if(csi_camera_create_event(&cam_ctx->event_handle,cam_ctx->cam_handle))
    {
            LOG_E("Fail to create event handler for cam %s\n",dev_name);
            goto TWO_ERR; 
    }

    cam_ctx->event_subscribe.type = CSI_CAMERA_EVENT_TYPE_CAMERA;
    cam_ctx->event_subscribe.id =  CSI_CAMERA_EVENT_WARNING | CSI_CAMERA_EVENT_ERROR;
    if(csi_camera_subscribe_event(cam_ctx->event_handle,&cam_ctx->event_subscribe))
    {
        LOG_E("Fail to subscribe eventfor cam %s\n",dev_name);
        goto TWO_ERR; 
    }


    for(loop_ch=0;loop_ch<cam_ctx->channel_num;loop_ch++)
    {
        cam_ctx->event_subscribe.type = CSI_CAMERA_EVENT_TYPE_CHANNEL0+loop_ch;
        cam_ctx->event_subscribe.id =  CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
        if(csi_camera_subscribe_event(cam_ctx->event_handle,&cam_ctx->event_subscribe))
        {
            LOG_E("Fail to subscribe eventfor cam %s\n",dev_name);
            goto TWO_ERR; 
        }
    }

	LOG_O("%s open successfully\n",dev_name);
	ret = pthread_create(&cam_ctx->cam_thread, NULL,
			     (void *)camera_event_thread, cam_ctx);
	if (ret != 0) {
		LOG_E("pthread_create() failed, ret=%d", ret);
		goto TWO_ERR;
	}

    if(params->type == CAM_TYEP_IR)
    {
            cb_context[0].id=0;
            cb_context[0].cam_handle = cam_ctx->cam_handle;
            cb_context[0].dsp_id = 1;
            cb_context[0].dsp_path = 3;    
            char test_set[16]="init";
            cb_context[0].buf_size = 1920*1080;
            cb_context[0].ref_buf=NULL;
            csi_camera_dsp_algo_param_t algo_param_1={
                .algo_name = "dsp1_dummy_algo_flo_1",
                .algo_cb.cb = dsp_algo_result_cb,
                .algo_cb.context = &cb_context[0],
                .algo_cb.arg_size =  sizeof(cb_args_t),
                .sett_ptr = test_set,
                .sett_size =sizeof(test_set),
                .extra_buf_num =1,
                .extra_buf_sizes = &cb_context[0].buf_size,
                .extra_bufs = &cb_context[0].ref_buf,
            };
            
            if(csi_camera_set_dsp_algo_param(cam_ctx->cam_handle, cb_context[0].dsp_id,cb_context[0].dsp_path, &algo_param_1))
            {
                LOG_E("set DSP algo fail\n");
                goto TWO_ERR;
            }

            cb_context[1].id=1;
            cb_context[1].cam_handle = cam_ctx->cam_handle;
            cb_context[1].dsp_id = 1;
            cb_context[1].dsp_path = 4;    
            cb_context[1].buf_size = 1920*1080;
            cb_context[1].ref_buf=NULL;
            csi_camera_dsp_algo_param_t algo_param_2={
                .algo_name = "dsp1_dummy_algo_flo",
                .algo_cb.cb = dsp_algo_result_cb,
                .algo_cb.context = &cb_context[1],
                .algo_cb.arg_size = sizeof(cb_args_t),
                .sett_ptr = test_set,
                .sett_size =sizeof(test_set),
                .extra_buf_num =1,
                .extra_buf_sizes = &cb_context[1].buf_size,
                .extra_bufs = &cb_context[1].ref_buf,
            };
            
            if(csi_camera_set_dsp_algo_param(cam_ctx->cam_handle, cb_context[1].dsp_id,cb_context[1].dsp_path, &algo_param_2))
            {
                LOG_E("set DSP algo fail\n");
                 goto TWO_ERR;
            }
            csi_camera_floodlight_led_set_flash_bright(cam_ctx->cam_handle, 500); //500ma
            csi_camera_projection_led_set_flash_bright(cam_ctx->cam_handle, 500); //500ma
            csi_camera_projection_led_set_mode(cam_ctx->cam_handle, LED_IR_ENABLE);
            csi_camera_floodlight_led_set_mode(cam_ctx->cam_handle, LED_IR_ENABLE);
            csi_camera_led_enable(cam_ctx->cam_handle, LED_FLOODLIGHT_PROJECTION);
    }

    // for(loop_ch=0;loop_ch<cam_ctx->channel_num;loop_ch++)
    // {
	//     csi_camera_channel_start(cam_ctx->cam_handle, cam_ctx->chn_cfg[loop_ch].chn_id);
    // }

	get_system_time(__func__, __LINE__);


    return cam_ctx;


TWO_ERR:
        for(loop_ch=0;loop_ch<cam_ctx->channel_num;loop_ch++)
        {
            csi_camera_channel_close(cam_ctx->cam_handle, cam_ctx->chn_cfg[loop_ch].chn_id);
        }
        csi_camera_close(cam_ctx->cam_handle);
ONE_ERR:
        free(cam_ctx);
        return NULL;
}

static void camera_close(camera_ctx_t *ctx)
{
    int loop_ch;

    if(ctx->cam_handle==NULL)
    {
        return;
    }

	if (pthread_join(ctx->cam_thread, NULL)) {
		LOG_E("pthread_join() failed\n");
	}

    csi_camera_unsubscribe_event(ctx->event_handle, &ctx->event_subscribe);

	csi_camera_destory_event(ctx->event_handle);
    // int loop_ch;
    // for(loop_ch=0;loop_ch<ctx->channel_num;loop_ch++)
    // {
    //     csi_camera_channel_stop(cam_ctx->cam_handle, cam_ctx->chn_cfg[loop_ch].chn_id);
    // }

    for(loop_ch=0;loop_ch<ctx->channel_num;loop_ch++)
    {
        csi_camera_channel_close(ctx->cam_handle, ctx->chn_cfg[loop_ch].chn_id);
    }
    csi_camera_close(ctx->cam_handle);
    free(ctx);
}


int main(int argc, char *argv[])
{

	char dev_name[128];
	int camera_id = 0;
	// 打印HAL接口版本号
	csi_api_version_u version;
	csi_camera_get_version(&version);
    camera_param_t params[MAX_CAM_NUM];
    camera_ctx_t * ctx[MAX_CAM_NUM]={NULL,NULL,NULL};
    int cam_num=0;
    bool running = false;
    cam_num =parseParams(argc,argv,params);
    if(cam_num <=0 || cam_num>MAX_CAM_NUM)
    {
        LOG_E("not camera is active\n");
        exit(0);
    }
    for(int i =0;i<cam_num;i++)
    {
        switch(params[i].type)
        {
            case CAM_TYEP_RGB:
                   ctx[i]=camera_open(&params[i]);
                   break;
            case CAM_TYEP_IR:
                   ctx[i]=camera_open(&params[i]);
                    break;
            default:
                 LOG_E("unspuoort camera :%d\n",i);
                 continue;
        }
        if(ctx[i]==NULL)
        {
            LOG_E("camera_%d open %d fail\n",i,params[i].type);
            goto ONR_ERR;
        }
        ctx[i]->cam_id = i;
        ctx[i]->frame_num =0;
        ctx[i]->exit = 0;
        camera_start_all(ctx[i]);
        running = true;
    }
    int all_exit;
    do{
        sleep(1);
        all_exit = 1;
        for(int i =0;i<cam_num;i++)
        {
            if(ctx[i]&&
               ctx[i]->exit==0 && 
               params[i].frames_to_stop>0 && 
                ctx[i]->frame_num >=params[i].frames_to_stop)
            {
                camera_stop_all(ctx[i]);
                ctx[i]->exit=1;
            }
            if(ctx[i])
                all_exit &= ctx[i]->exit;
        }
        if(all_exit ==1)
        {
            LOG_O("all_exit\n");
            running = false;
        }

    }while(running);
    
ONR_ERR:
    for(int i =0;i<cam_num;i++)
    {
        if(ctx[i])
            camera_close(ctx[i]);
    }
    exit(0);
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


	csi_camera_frame_get_meta_unit(
		&meta_unit, meta_data, CSI_CAMERA_META_ID_FRAME_ID);
	LOG_I("meta_id=%d, meta_type=%d, meta_value=%d\n",
			meta_unit.id, meta_unit.type, meta_unit.int_value);
}

static void dump_camera_frame(csi_frame_s *frame, csi_pixel_fmt_e fmt, int chan_id)
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

              
	sprintf(file,"demo_save_img%s_%d_%d",meta_unit.str_value,chan_id, file_indx++%10);
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
                            LOG_O("img fomrat is not match with frame planes:%d\n",frame->img.num_planes);
                            return;
                        }
                        break;
                case CSI_PIX_FMT_YUV_SEMIPLANAR_422:
                        if(frame->img.num_planes ==2)
                        { 
                            buf_size+=frame->img.strides[1]*frame->img.height;
                        }
                        else{
                            LOG_O("img fomrat is not match with frame planes:%d\n",frame->img.num_planes);
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
