#pragma once

#include <QObject>
#include <QDir>

class FinalizerCreater : public QObject
{
	Q_OBJECT

public:
	FinalizerCreater(): secStart(0) { ffmpegPath = QDir::current().filePath("ffmpeg"); }
	~FinalizerCreater();

	void generateCommand(QString filename);
	void setSourceFile(QString name) { sourceFile = name; }
	void setTargetName(QString name) { targetFile = name; }
	void setStart(double t) { secStart = t; }
//	void setEnd(double t) { secEnd = t; }

private:
	QString sourceFile;
	QString targetFile;
	double secStart;
//	double secEnd;
	QString ffmpegPath;
};
