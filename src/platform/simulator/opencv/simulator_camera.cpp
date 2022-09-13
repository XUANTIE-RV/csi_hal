#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
using namespace cv;
using namespace std;

#define LOG_LEVEL 2
#include <syslog.h>
#include "simulator_camera.h"

VideoCapture capture;
#define local_yuv_height 720
#define local_yuv_width 1280
#define PROPERTY_NUM 100

//char filename[] = "./examples/resource/yuv420_1280x720_csky2016.yuv";
char filename[] = "../../../examples/resource/yuv420_1280x720_csky2016.yuv";
static void swapyuv_i420tonv12(unsigned char *i420bytes,
			       unsigned char *nv12bytes, int height, int width)
{
	int nleny = height * width;
	int nlenu = nleny / 4;
	memcpy(nv12bytes, i420bytes, height * width);
	for (int i = 0; i < nlenu; i++) {
		nv12bytes[nleny + 2 * i] = i420bytes[nleny + i]; //U
		nv12bytes[nleny + 2 * i + 1] = i420bytes[nleny + nlenu + i]; //V
	}
}
int csi_camera_opencv_get_picture_local(csi_pixel_fmt_e fmt, int height,
					int width, unsigned char *data,
					csi_camera_property_description_s *property_description)
{
	int ret = 0;
	int framesize = local_yuv_width * local_yuv_height * 3 / 2;
	Mat local_mat_i420, mat_i420, mat_nv12, mat_bgr, mat_channel;
	local_mat_i420.create(local_yuv_height * 3 / 2, local_yuv_width, CV_8UC1);
	mat_i420.create(height * 3 / 2, width, CV_8UC1);
	mat_nv12.create(height * 3 / 2, width, CV_8UC1);
	mat_bgr.create(height, width, CV_8UC3);
	FILE *fileIn = fopen(filename, "rb+");
	fread(local_mat_i420.data, framesize * sizeof(unsigned char), 1, fileIn);
	cvtColor(local_mat_i420, mat_channel, COLOR_YUV2BGR_I420);
	resize(mat_channel, mat_bgr, mat_bgr.size());
	framesize = height * width;
	Size mat_bgr_sz(mat_bgr.cols, mat_bgr.rows);
	Point2f center(static_cast<float>(mat_bgr.cols / 2.),
		       static_cast<float>(mat_bgr.rows / 2.));
	Mat rot_mat;
	for (int i = 0; i < PROPERTY_NUM; i++) {
		switch (property_description[i].id) {
		case CSI_CAMERA_PID_HFLIP:
			if (property_description[i].value.bool_value == true) {
				flip(mat_bgr, mat_bgr, 1);
			}
			break;
		case CSI_CAMERA_PID_VFLIP:
			if (property_description[i].value.bool_value == true) {
				flip(mat_bgr, mat_bgr, 0);
			}
			break;
		case CSI_CAMERA_PID_ROTATE:
			rot_mat = getRotationMatrix2D(center, property_description[i].value.int_value,
						      1.0);
			warpAffine(mat_bgr, mat_bgr, rot_mat, mat_bgr_sz, INTER_LINEAR,
				   BORDER_REPLICATE);
			break;
		case CSI_CAMERA_PID_EXPOSURE_ABSOLUTE:
			for (int row = 0; row < mat_bgr.rows; row++) {
				for (int col = 0; col < mat_bgr.cols; col++) {
					mat_bgr.at<Vec3b>(row, col)[0] = mat_bgr.at<Vec3b>(row,
									 col)[0] + property_description[i].value.int_value;
					mat_bgr.at<Vec3b>(row, col)[1] = mat_bgr.at<Vec3b>(row,
									 col)[1] + property_description[i].value.int_value;
					mat_bgr.at<Vec3b>(row, col)[2] = mat_bgr.at<Vec3b>(row,
									 col)[2] + property_description[i].value.int_value;
				}
			}
			break;
		case CSI_CAMERA_PID_RED_GAIN:
			for (int row = 0; row < mat_bgr.rows; row++) {
				for (int col = 0; col < mat_bgr.cols; col++) {
					mat_bgr.at<Vec3b>(row, col)[2] = mat_bgr.at<Vec3b>(row,
									 col)[2] + property_description[i].value.int_value;
				}
			}
			break;
		case CSI_CAMERA_PID_GREEN_GAIN:
			for (int row = 0; row < mat_bgr.rows; row++) {
				for (int col = 0; col < mat_bgr.cols; col++) {
					mat_bgr.at<Vec3b>(row, col)[1] = mat_bgr.at<Vec3b>(row,
									 col)[1] + property_description[i].value.int_value;
				}
			}
			break;
		case CSI_CAMERA_PID_BLUE_GAIN:
			for (int row = 0; row < mat_bgr.rows; row++) {
				for (int col = 0; col < mat_bgr.cols; col++) {
					mat_bgr.at<Vec3b>(row, col)[0] = mat_bgr.at<Vec3b>(row,
									 col)[0] + property_description[i].value.int_value;
				}
			}
			break;
		default:
			break;
		}
	}
	switch (fmt) {
	case CSI_PIX_FMT_I420:
		framesize = framesize * 3 / 2;
		cvtColor(mat_bgr, mat_i420, COLOR_BGR2YUV_I420);
		memcpy(data, mat_i420.data, framesize * sizeof(unsigned char));
		break;
	case CSI_PIX_FMT_NV12:
		framesize = framesize * 3 / 2;
		cvtColor(mat_bgr, mat_i420, COLOR_BGR2YUV_I420);
		swapyuv_i420tonv12(mat_i420.data, mat_nv12.data, height, width);
		memcpy(data, mat_nv12.data, framesize * sizeof(unsigned char));
		break;
	case CSI_PIX_FMT_BGR:
		framesize = framesize * 3;
		memcpy(data, mat_bgr.data, framesize * sizeof(unsigned char));
		break;
	default:
		ret = -1;
		break;
	}

	mat_channel.release();
	mat_i420.release();
	mat_nv12.release();
	mat_bgr.release();
	rot_mat.release();
	fclose(fileIn);
	return ret;
}
int csi_camera_opencv_get_picture(int height, int width, unsigned char *data)
{
	Mat mat_channel,tmp;
	mat_channel.create(height,width,CV_8UC3);
	capture >> tmp;
	resize(tmp,mat_channel,mat_channel.size());
	memcpy(data, mat_channel.data, height * width * 3 * sizeof(unsigned char));
	return 0;
}
int csi_camera_opencv_format_picture(csi_pixel_fmt_e fmt, int dst_height,
				  int dst_width, unsigned char *data_dst,
				  int src_height, int src_width, unsigned char *data_src,
				  csi_camera_property_description_s *property_description)
{
	Mat mat_channel, mat_i420, mat_nv12,mat_tmp;
	int ret = 0;
	int framesize = dst_height * dst_width;
	mat_i420.create(dst_height * 3 / 2, dst_width, CV_8UC1);
	mat_nv12.create(dst_height * 3 / 2, dst_width, CV_8UC1);
	mat_channel.create(dst_height, dst_width, CV_8UC3);
	mat_tmp.create(src_height,src_width,CV_8UC3);
	memcpy(mat_tmp.data,data_src,src_height * src_width * 3 * sizeof(unsigned char));
	resize(mat_tmp, mat_channel, mat_channel.size());

	/*reserve to debug
	printf("mat_channel.width = %d, mat_channel.height = %d\n",mat_channel.cols,mat_channel.rows);
	imshow("mat_channel",mat_channel);
	waitKey(0);
	*/
	Size mat_channel_sz(mat_channel.cols, mat_channel.rows);
	Point2f center(static_cast<float>(mat_channel.cols / 2.),
		       static_cast<float>(mat_channel.rows / 2.));
	Mat rot_mat;
	for (int i = 0; i < PROPERTY_NUM; i++) {
		switch (property_description[i].id) {
		case CSI_CAMERA_PID_HFLIP:
			if (property_description[i].value.bool_value == true) {
				flip(mat_channel, mat_channel, 1);
			}
			break;
		case CSI_CAMERA_PID_VFLIP:
			if (property_description[i].value.bool_value == true) {
				flip(mat_channel, mat_channel, 0);
			}
			break;
		case CSI_CAMERA_PID_ROTATE:
			rot_mat = getRotationMatrix2D(center, property_description[i].value.int_value,
						      1.0);
			warpAffine(mat_channel, mat_channel, rot_mat, mat_channel_sz, INTER_LINEAR,
				   BORDER_REPLICATE);
			break;
		default:
			break;
		}
	}
	switch (fmt) {
	case CSI_PIX_FMT_I420:
		framesize = framesize * 3 / 2;
		cvtColor(mat_channel, mat_i420, COLOR_BGR2YUV_I420);
		memcpy(data_dst, mat_i420.data, framesize * sizeof(unsigned char));
		break;
	case CSI_PIX_FMT_NV12:
		framesize = framesize * 3 / 2;
		cvtColor(mat_channel, mat_i420, COLOR_BGR2YUV_I420);
		swapyuv_i420tonv12(mat_i420.data, mat_nv12.data, dst_height, dst_width);
		memcpy(data_dst, mat_nv12.data, framesize * sizeof(unsigned char));
		break;
	case CSI_PIX_FMT_BGR:
		framesize = framesize * 3;
		memcpy(data_dst, mat_channel.data, framesize * sizeof(unsigned char));
		break;
	default:
		ret = -1;
		break;
	}
	mat_channel.release();
	mat_i420.release();
	mat_nv12.release();
	rot_mat.release();
	return ret;
}
int camera_open_opencv(int idx)
{
	bool ret = capture.open(idx);
	if (ret != true) {
		return -1;
	}
	return  0;
}
int camera_property_set_opencv(csi_camera_property_s *property)
{
	int ret = 0;
	switch (property->id) {
	case CSI_CAMERA_PID_HFLIP:
		break;
	case CSI_CAMERA_PID_VFLIP:
		break;
	case CSI_CAMERA_PID_ROTATE:
		break;
	case CSI_CAMERA_EXPOSURE_MODE_AUTO:
		if (property->value.enum_value == CSI_CAMERA_EXPOSURE_MODE_AUTO) {
			capture.set(CV_CAP_PROP_AUTO_EXPOSURE, true);
		} else if (property->value.enum_value == CSI_CAMERA_EXPOSURE_MANUAL) {
			capture.set(CV_CAP_PROP_AUTO_EXPOSURE, false);
		} else if (property->value.enum_value == CSI_CAMERA_EXPOSURE_SHUTTER_PRIORITY) {
			capture.set(CV_CAP_PROP_AUTO_EXPOSURE, false);
		} else if (property->value.enum_value ==
			   CSI_CAMERA_EXPOSURE_APERTURE_PRIORITY) {
			capture.set(CV_CAP_PROP_AUTO_EXPOSURE, false);
		} else {
			ret - 1;
		}
		break;
	case CSI_CAMERA_PID_EXPOSURE_ABSOLUTE:
		capture.set(CV_CAP_PROP_BRIGHTNESS, property->value.int_value);
		break;
	case CSI_CAMERA_PID_3A_LOCK:
		if (property->value.bitmask_value & CSI_CAMERA_LOCK_EXPOSURE) {
			capture.set(CV_CAP_PROP_AUTO_EXPOSURE, false);
		} else if (property->value.bitmask_value & CSI_CAMERA_LOCK_WHITE_BALANCE) {
			capture.set(CV_CAP_PROP_TEMPERATURE, false);
		} else if (property->value.bitmask_value & CSI_CAMERA_LOCK_FOCUS) {
			LOG_D("supportted by lens\n");
		} else {
			ret = -1;
		}
		break;
	case CSI_CAMERA_PID_RED_GAIN:
		capture.set(CV_CAP_PROP_WHITE_BALANCE_RED_V, property->value.int_value);
		break;
	case CSI_CAMERA_PID_GREEN_GAIN:
		capture.set(CV_CAP_PROP_GAIN, property->value.int_value);
		break;
	case CSI_CAMERA_PID_BLUE_GAIN:
		capture.set(CV_CAP_PROP_WHITE_BALANCE_BLUE_U, property->value.int_value);
		break;
	default:
		break;
	}
	return ret;
}
int camera_cfg_opencv(csi_cam_handle_t cam_handle, int height, int width)
{
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, height);
	capture.set(CV_CAP_PROP_FRAME_WIDTH, width);
	return 0;
}
int camera_set_mode_opencv(csi_camera_mode_cfg_s *cfg)
{
	int ret = 0;
	switch (cfg->mode_id) {
	case 1:
		capture.set(CV_CAP_PROP_FPS, 30);
		break;
	case 2:
		capture.set(CV_CAP_PROP_FPS, 10);
		break;
	default:
		capture.set(CV_CAP_PROP_FPS, 20);
		ret = -1;
		break;
	}
	return ret;
}
int camera_close_opencv(csi_cam_handle_t cam_handle)
{
	capture.release();
	return 0;
}
//cannot find macro and use this function temporarily and change in the future
int camera_image_size_opencv(csi_frame_s *frame)
{
	int ret = 0;
	switch (frame->img.pix_format) {
	case CSI_PIX_FMT_I420:
		ret = frame->img.height * frame->img.width  * 3 / 2;
		break;
	case CSI_PIX_FMT_NV12:
		ret = frame->img.height * frame->img.width  * 3 / 2;
		break;
	case CSI_PIX_FMT_BGR:
		ret = frame->img.height * frame->img.width  * 3;
		break;
	default:
		LOG_E("Unkonw pix_format:%d, return image size = 0\n",
		      frame->img.pix_format);
		ret = 0;
		break;
	}
	return ret;
}
