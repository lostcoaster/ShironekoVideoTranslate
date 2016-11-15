#include "markdetector.h"

MarkDetector::MarkDetector()
	: countThree(0), subtitleStarted(false) {
	erodeElement = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
	dilateElement = getStructuringElement(cv::MORPH_RECT, cv::Size(6, 6));
	pinPointX = pinPointY = 0.5;
}

MarkDetector::~MarkDetector() {
}

bool MarkDetector::detectTriangle(cv::Mat m) {
	auto roiRect = cv::Rect(0, ceil(m.rows * 0.45), m.cols, floor(m.rows * 0.55));
	auto roi = m(roiRect);
	cv::Mat procing;
	cvtColor(roi, procing, cv::COLOR_BGR2HSV);
	inRange(procing, cv::Scalar(18, 200, 224), cv::Scalar(30, 255, 255), procing);
	erode(procing, procing, erodeElement);
	dilate(procing, procing, dilateElement);

	std::vector<std::vector<cv::Point>> contour;
	std::vector<cv::Vec4i> hie;
	cv::findContours(procing, contour, hie, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	if (hie.size() == 1) {
		auto e = 0.1 * cv::arcLength(contour[0], true);
		std::vector<cv::Point> appr;
		cv::approxPolyDP(contour[0], appr, e, true);
		if(appr.size()==3) {
			countThree = 0;
			return true;
		} else {
			if (++countThree >= 3) {
				countThree = 0;
				return true;
			} else {
				return false;
			}
		}
	}
	countThree = 0;
	return false;
}

bool MarkDetector::detectColorChanged(cv::Mat m) {
	// I tried contour, I tried moment, and it turns out the simplest is the best
//	auto currentColor = m.at<cv::Vec3b>(m.rows * pinPointY, m.cols * pinPointX);
//	for (int i = 0; i < 3; ++i) {
//		if (abs(currentColor[i] - lastColor[i]) > 5) {
//			lastColor = currentColor;
//			return true;
//		}
//	}
//	return false;

	// use better scanline method
	cv::Mat scanline;
	cv::cvtColor(m(cv::Range(m.rows*0.45, m.rows), cv::Range(m.cols / 2 - 1, m.cols / 2)), scanline, CV_BGR2GRAY);
	bool ret = false;
	if (!lastScanline.empty()) {
		cv::absdiff(scanline, lastScanline, lastScanline);
		ret = (cv::sum(lastScanline) / lastScanline.rows)[0] > 20;
	}
	lastScanline = scanline;
	return ret;
}

void MarkDetector::processImage(cv::Mat m) {
	if(subtitleStarted && detectTriangle(m)) {
		subtitleStarted = false;
		detectColorChanged(m); // take a snapshot
		emit subtitleStop();
	} else if (!subtitleStarted && detectColorChanged(m)) {
		subtitleStarted = true;
		emit subtitleStart();
	}
}

