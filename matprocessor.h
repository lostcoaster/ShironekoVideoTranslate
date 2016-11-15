#pragma once

#include <QObject>
#include <opencv2/opencv.hpp>

class MatProcessor : public QObject {
	Q_OBJECT
public:
	virtual void processImage(cv::Mat) = 0;
};