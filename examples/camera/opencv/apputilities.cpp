#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
using namespace cv;
using namespace std;

#include "apputilities.h"

int show_frame_image(void *mat, int height, int width)
{
	Mat dis_mat;
	dis_mat.create(height * 3 / 2, width, CV_8UC1);
	dis_mat.data = (uchar *)mat;
	cvtColor(dis_mat, dis_mat, CV_YUV2BGRA_I420);
	namedWindow("camera local i420");
	imshow("camera local i420", dis_mat);
	waitKey(1);
	dis_mat.release();
	return 0;
}

