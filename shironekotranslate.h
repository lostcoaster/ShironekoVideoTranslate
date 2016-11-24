#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_shironekotranslate.h"
#include "ShironekoVideoPlayer.h"
#include "SubtitleCreator.h"
#include "markdetector.h"

class ShironekoTranslate : public QMainWindow {
	Q_OBJECT

public:
	ShironekoTranslate(QWidget* parent = Q_NULLPTR);

private:
	bool firstSubtitle;
	QString sourceFile, destinationFile;

	Ui::ShironekoTranslateClass ui;

	ShironekoVideoPlayer player;
	std::shared_ptr<MarkDetector> detector;
	std::unique_ptr<SubtitleCreator> subtitle;

	int finishCountDown;
	void finishVideo();
	bool fixVariableFps();

	void initConnections();
	void selectAndLoadVideo();

	signals:
	void subtitleChanged(QImage);
	void quitApplication();

private slots:
	void updateImage(QImage, QImage);
	void showInfos(QString info) const;
	void startPauseVideo();
	void startSubstitledVideo();
	void startSubtitle();
	void stopLastSubtitle();
	void finish();
	void reportError(QString error);
	void initialize();
};
