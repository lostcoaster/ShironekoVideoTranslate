#include "ShironekoVideoPlayer.h"
#include <QtConcurrent/QtConcurrent>
#include <QMessageBox>
#include <QDir>
#include <QtDebug>
#include "markdetector.h"

ShironekoVideoPlayer::ShironekoVideoPlayer(QObject* parent): QObject(parent), frameRate(0), codec(-1), playing(false), recording(false) {
}

ShironekoVideoPlayer::~ShironekoVideoPlayer() {
	capture.release();
	playing = false;
}

ShironekoVideoPlayer::OpenState ShironekoVideoPlayer::loadVideo(QString file, QString output) {
	capture.open(file.toStdString());
	if (!capture.isOpened()) {
		return NoDecoder;
	}
	frameRate = capture.get(CV_CAP_PROP_FPS);
	if (frameRate > 100.0) {
		capture.release();
		return VaribalFPS;
	}
	// cut right 5 pixels, as they are black border from nowhere
	sourceSize = cv::Size(capture.get(CV_CAP_PROP_FRAME_WIDTH) - 5, capture.get(CV_CAP_PROP_FRAME_HEIGHT));
	cropPart = cv::Rect(0, 0, sourceSize.width, sourceSize.height * 0.45);
	if (codec == -1) {
		codec = checkCodecs();
	}
	if (codec == -1) {
		capture.release();
		return NoEncoder;
	}
	if (!record) {
		record.reset(new cv::VideoWriter());
		if (!record->open(output.toStdString(), codec, frameRate, sourceSize)) {
			capture.release();
			return WriteError;
		}
	}
	
	processingTimer.start();
	return Success;
}

void ShironekoVideoPlayer::releaseAll() {
	if(record) {
		record->release();
	}
	if(capture.isOpened()) {
		capture.release();
	}
}

void ShironekoVideoPlayer::addProcessor(std::shared_ptr<MatProcessor> p) {
	processors.push_back(p);
}

int ShironekoVideoPlayer::checkCodecs() {
	//run it here , to signal failure as soon as possible
	record.reset(new cv::VideoWriter());
	//try codecs, if everything fails , MJPG
	QList<int> codecsOptions{
		CV_FOURCC('H', '2', '6', '4'), // h264, first choice
		CV_FOURCC('X', '2', '6', '4'),
		CV_FOURCC('D', 'I', 'V', 'X'),
		CV_FOURCC('M', 'P', '4', 'V'), // h263, ok
		CV_FOURCC('W', 'V', 'P', '2'), // windows media player things
		CV_FOURCC('W', 'M', 'V', '3'),
		CV_FOURCC('M', 'J', 'P', 'G'), // eww, mjpg
		CV_FOURCC('D', 'I', 'B', ' '), // the most stupid uncompressed
	};
	QTemporaryDir dir;
	std::string filename = QDir::toNativeSeparators(QDir(dir.path()).filePath("testcodec.avi")).toStdString();
	for (auto c : codecsOptions) {
		if (record->open(filename, c, frameRate, sourceSize)) {
			record->release();
			record.reset();
			return c;
		}
	}
	return -1;
}

void ShironekoVideoPlayer::playVideo() {
	if (playing) {
		emit infoReport(tr("Already playing"));
		return; // already playing
	}
	if (!capture.isOpened()) {
		emit errorReport(tr("Video not loaded"));
		return; // not loaded
	}
	playing = true;
	emit videoPlayingStateChange(playing);

	onFrameTimerTick();
}

void ShironekoVideoPlayer::pauseVideo() {
	playing = false;
	emit videoPlayingStateChange(playing);
}

void ShironekoVideoPlayer::onFrameTimerTick() {
	cv::Mat m;
	if (playing && capture.read(m)) {
		//process the mat before rendering

		//		if (recording) {
		subtitleImageMutex.lock();
		QImage proceeded;
		if (!subtitleImage.empty()) {
			cv::Mat result(sourceSize, CV_8UC3);
			m(cropPart).copyTo(result(cv::Rect(0, 0, sourceSize.width, cropPart.height)));
			subtitleImage.copyTo(result(cv::Rect(0, cropPart.height, sourceSize.width, sourceSize.height - cropPart.height)));
			*record << result;
			qDebug() << "Write 1 frame";
			cv::cvtColor(result, result, CV_BGR2RGB);
			proceeded = QImage(result.data, result.cols, result.rows, result.step, QImage::Format_RGB888).copy();
		} else {
			qDebug() << "No sub frame, skipped";
			//				*record << m(cv::Rect(0, 0, sourceSize.width, sourceSize.height));
		}
		subtitleImageMutex.unlock();
		//		} else {
		for (auto p : processors) {
			p->processImage(m);
		}

		cv::cvtColor(m, convertedImage, CV_BGR2RGB);
		QImage full(convertedImage.data, convertedImage.cols, convertedImage.rows, convertedImage.step, QImage::Format_RGB888);
		emit newFrame(full, proceeded);

		int nextFrameTime = 1000 / frameRate - processingTimer.elapsed();
		nextFrameTimer.singleShot(nextFrameTime > 0? nextFrameTime : 0, this, SLOT(onFrameTimerTick()));
		processingTimer.start();
		//		}
	} else {
		playing = false; // for the case capture is complete
	}
}

void ShironekoVideoPlayer::onSubtitleFrameArrive(QImage img) {
	subtitleImageMutex.lock();
	auto newImg = img.convertToFormat(QImage::Format_RGB888);
	subtitleImage = cv::Mat(newImg.height(), newImg.width(), CV_8UC3, newImg.bits(), newImg.bytesPerLine());
	cv::resize(subtitleImage, subtitleImage, cv::Size(sourceSize.width, sourceSize.height - cropPart.height));
	cv::cvtColor(subtitleImage, subtitleImage, CV_RGB2BGR);
	subtitleImageMutex.unlock();
}
