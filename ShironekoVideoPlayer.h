#pragma once
#include "matprocessor.h"
#include <QObject>
#include <QImage>

#include "opencv2/opencv.hpp"
#include <memory>
#include <QTimer>
#include <QTime>
#include <QMutex>

/*todo : problems
 1. resized arrows can be detected (maybe we should use color filtering)problem: 
 2. size dismatch (well... just use ffmpeg to crop)
 3. log start time for crop

*/
class ShironekoVideoPlayer : public QObject {
	Q_OBJECT
public:
	ShironekoVideoPlayer();;
	~ShironekoVideoPlayer();

	bool loadVideo(QString file, QString output = "");
	double getFrameRate() const { return frameRate; }
	int getFrameNumber() { return capture.get(CV_CAP_PROP_POS_FRAMES); }
	int getTotalFrameNumber() { return capture.get(CV_CAP_PROP_FRAME_COUNT); }
	QSize getFrameSize() const { return QSize(sourceSize.width, sourceSize.height); }
	bool isPlaying() const { return playing; }
	void startRecord() { playing = true; recording = true; subtitleImage.release(); }
	void triggerFrameManually() { if(recording) onFrameTimerTick(); }

	void addProcessor(std::shared_ptr<MatProcessor> p);
protected:
	//	virtual void processImage(cv::Mat m);
	//
	//	virtual int detectTriangle(cv::Mat m);

private:
	std::list<std::shared_ptr<MatProcessor>> processors;

	QImage img;
	cv::VideoCapture capture;
	std::unique_ptr<cv::VideoWriter> record;
	double frameRate;
	int codec;

	bool playing;
	cv::Mat convertedImage;
	cv::Mat subtitleImage;
	cv::Rect cropPart;
	QTimer nextFrameTimer;

	cv::Size sourceSize;
	bool recording;
	QMutex subtitleImageMutex;
private:
	void checkCodecs();
public slots:
	//void processedMat(cv::Mat m);
	void playVideo();
	void pauseVideo();
	void onSubtitleFrameArrive(QImage img);

private slots:
	void onFrameTimerTick();

	signals :
	void errorReport(const QString what);
	void infoReport(const QString what);
	void newFrame(QImage, QImage);
	void videoPlayingStateChange(bool playing);
};
