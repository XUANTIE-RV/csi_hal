/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: fuqian.zxr <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
using namespace cv;
using namespace std;

#include "camera_utilities.h"


int image_save_opencv_i420(void *mat, int height, int width)
{
	static int save_mat_num = 0;
	char output_filename[255];
	int buflen = height * width * 3 / 2;
	sprintf(output_filename, "/tmp/%d_i420.yuv", save_mat_num);
	FILE *pFileOut = fopen(output_filename, "wb");
	fwrite(mat, buflen * sizeof(unsigned char), 1, pFileOut);
	save_mat_num++;
	fclose(pFileOut);
	return 0;
}

int image_display_opencv_i420(void *mat, int height, int width)
{
	Mat dis_mat;
	dis_mat.create(height * 3 / 2, width, CV_8UC1);
	dis_mat.data = (uchar *)mat;
	cvtColor(dis_mat, dis_mat, CV_YUV2BGRA_I420);
	namedWindow("camera i420");
	imshow("camera i420", dis_mat);
	waitKey(1);
	dis_mat.release();
	return 0;
}

int image_save_opencv_nv12(void *mat, int height, int width)
{
	static int save_mat_num = 0;
	char output_filename[255];
	int buflen = height * width * 3 / 2;
	sprintf(output_filename, "/tmp/%d_nv12.yuv", save_mat_num);
	FILE *pFileOut = fopen(output_filename, "wb");
	fwrite(mat, buflen * sizeof(unsigned char), 1, pFileOut);
	save_mat_num++;
	fclose(pFileOut);
	return 0;
}

int image_display_opencv_nv12(void *mat, int height, int width)
{
	Mat dis_mat;
	dis_mat.create(height * 3 / 2, width, CV_8UC1);
	dis_mat.data = (uchar *)mat;
	cvtColor(dis_mat, dis_mat, CV_YUV2BGRA_NV12);
	namedWindow("camera nv12");
	imshow("camera nv12", dis_mat);
	waitKey(1);
	dis_mat.release();
	return 0;
}

int image_save_opencv_bgr(void *mat, int height, int width)
{
	static int save_mat_num = 0;
	char output_filename[255];
	int buflen = height * width * 3;
	sprintf(output_filename, "/tmp/%d.bgr", save_mat_num);
	FILE *pFileOut = fopen(output_filename, "wb");
	fwrite(mat, buflen * sizeof(unsigned char), 1, pFileOut);
	save_mat_num++;
	return 0;
}

int image_display_opencv_bgr(void *mat, int height, int width)
{
	Mat dis_mat;
	dis_mat.create(height, width, CV_8UC3);
	dis_mat.data = (uchar *)mat;
	namedWindow("camera");
	imshow("camera", dis_mat);
	waitKey(1);
	dis_mat.release();
	return 0;

}
