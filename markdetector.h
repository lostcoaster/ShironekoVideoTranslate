#pragma once

#include "matprocessor.h"

class MarkDetector : public MatProcessor
{
	Q_OBJECT
public:
	MarkDetector();
	~MarkDetector();
	virtual void processImage(cv::Mat) override;
	void initialize() { countThree = 0; subtitleStarted = true; emit subtitleStart(); }
	void finish() { emit subtitleStop(); }
protected:
	int countThree; // count to 3 and if you are still true...
//	bool showingMark;
	cv::Mat erodeElement, dilateElement;
	double pinPointX, pinPointY; // where we use to detect subtitle change
//	cv::Vec3b lastColor;
	cv::Mat lastScanline;
	bool subtitleStarted;

	virtual bool detectTriangle(cv::Mat m);
	virtual bool detectColorChanged(cv::Mat m);

	signals:
	void subtitleStart();
	void subtitleStop();
};
