#include "ffmpegdirector.h"
#include <qprocess.h>
#include <QDir>
#include <qtextstream.h>
#include <QDebug>
#include <QMessageBox>

FfmpegDirector::FfmpegDirector(QObject* parent, QString ffmpegPath)
	: QObject(parent), ffmpegPath(ffmpegPath), totalSeconds(-1) {
}

FfmpegDirector::~FfmpegDirector() {
}

void FfmpegDirector::convertToFixedFrameRate(const QString & sourceFile, double fps, const QString & destFile) {
	if (p && p->state() == QProcess::Running) {
		return; // can't run twice
	}
	p = std::make_unique<QProcess>(this);
	p->setProcessChannelMode(QProcess::MergedChannels);
	QObject::connect(p.get(), SIGNAL(readyRead()), this, SLOT(onConvertToFixFrameRateUpdate()));
	QObject::connect(p.get(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onConvertToFixFrameRateUpdate()));
	QObject::connect(p.get(), SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(error()));
	QStringList ss;
	ss.append("-y");
	ss.append("-i");
	ss.append(QDir::toNativeSeparators(sourceFile));
	ss.append("-vsync");
	ss.append("1");
	ss.append("-r");
	ss.append(QString::number(fps));
	ss.append(QDir::toNativeSeparators(destFile)); // output file
	p->start(ffmpegPath, ss);
}

void FfmpegDirector::addAudioAndEncode(const QString& sourceFile, const QString& destFile, const QMap<QString, double>& parameters) {
	if (p && p->state()==QProcess::Running) {
		return; // can't run twice
	}
	double audioStartTime = parameters.value("audioStart", 0.0);
	double durationTime = parameters.value("duration", -1.0);
	QString finalFile = destFile.left(destFile.lastIndexOf('.')) + ".mp4"; // convert to mp4 file
	p = std::make_unique<QProcess>(this);
	p->setProcessChannelMode(QProcess::MergedChannels);

	QObject::connect(p.get(), SIGNAL(readyRead()), this, SLOT(onConvertToFixFrameRateUpdate()));
	QObject::connect(p.get(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onConvertToFixFrameRateUpdate()));
	QObject::connect(p.get(), SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(error()));
	QStringList ss;
	ss.append("-y");
	ss.append("-ss");
	ss.append(QString::number(audioStartTime));
	if(durationTime > 0) {
		ss.append("-t");
		ss.append(QString::number(durationTime));
		totalSeconds = durationTime;
	}
	ss.append("-i");
	ss.append(QDir::toNativeSeparators(sourceFile));
	ss.append("-i");
	ss.append(QDir::toNativeSeparators(destFile));
	ss.append("-map");
	ss.append("0:a");
	ss.append("-map");
	ss.append("1:v");
	ss.append("-c:a");
	ss.append("aac");
	ss.append("-c:v");
	ss.append("libxvid");
	ss.append("-q:v");
	ss.append("5");
	ss.append("-q:a");
	ss.append("-3");
	ss.append("-filter:v");
	ss.append("setpts=PTS-STARTPTS");
	ss.append("-shortest");
	ss.append(QDir::toNativeSeparators(finalFile));

	p->start(ffmpegPath, ss);
}

void FfmpegDirector::waitForFinish(int msec) {
	if(p && p->state() == QProcess::Running) {
		if (msec > 0) {
			p->waitForFinished(msec);
		} else {
			p->waitForFinished();
		}
	}
}

namespace {
	double timeStrToSec(QString s) {
		QStringList timeparts = s.split(":");
		return timeparts[0].toInt() * 3600 + timeparts[1].toInt() * 60 + timeparts[2].toDouble();
	}
}

void FfmpegDirector::onConvertToFixFrameRateUpdate() {
	if (p->state() == QProcess::NotRunning) {
		// "finished" signal
		emit updateProgress(1.1); // a bit larger than max
		totalSeconds = -1;
		QObject::disconnect(p.get(), nullptr, this, nullptr); // disconnect
	} else if (p->canReadLine()) {
		QTextStream t(p.get());
		QString line;
		while (!(line = t.readLine()).isEmpty()) {
			line = line.trimmed();
			qDebug() << "LINE: " << line;
			if (totalSeconds < 0 && line.startsWith("Duration")) {
				totalSeconds = timeStrToSec(line.mid(10, 11));
			} else if (totalSeconds > 0 && line.startsWith("frame= ")) {
				double currentSec = timeStrToSec(line.mid(line.indexOf("time=") + 5, 11));
				emit updateProgress(currentSec / totalSeconds * 100);
			}
		}
	}
}

void FfmpegDirector::error() {
	qDebug() << "Found Error";
}
