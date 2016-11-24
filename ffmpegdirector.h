#pragma once

#include <QObject>
#include <qprocess.h>
#include <memory>

class FfmpegDirector : public QObject
{
	Q_OBJECT

public:
	FfmpegDirector(QObject* parent, QString ffmpegPath);
	~FfmpegDirector();

	void convertToFixedFrameRate(const QString & sourceFile, double fps, const QString & destFile);
	void addAudioAndEncode(const QString & sourceFile, const QString & destFile, const QMap<QString, double>& parameters);
	void waitForFinish(int msec = -1);
	bool isFinished() const { return (!p || p->state() == QProcess::NotRunning); }
private:
	QString ffmpegPath;
	std::unique_ptr<QProcess> p;

	// converting fps variables:
	double totalSeconds;

	private slots:
	void onConvertToFixFrameRateUpdate();
	void error();

	signals:
	void updateProgress(int percent);
};
