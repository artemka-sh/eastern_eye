#pragma once

#include "types.hpp"
#include <opencv2/videoio.hpp>

void setupTrackbars(cv::VideoCapture& cap, int& speed);
void updatePositionTrackbar(cv::VideoCapture& cap);
void printControls();
void printFrameStats(int frameNum, const SystemStats& stats, bool lineStopped);
