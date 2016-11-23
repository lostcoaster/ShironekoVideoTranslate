#include "finalizercreater.h"
#include <QFile>
#include <qtextstream.h>

FinalizerCreater::~FinalizerCreater() {
}

void FinalizerCreater::generateCommand(QString filename) {
	QFile f(filename);
	f.open(QIODevice::WriteOnly);
	QTextStream fs(&f);
	QFileInfo fi(targetFile);
	fs << "@echo off\r\n";
	fs << "SET ffmpegPath=\"" << QDir::toNativeSeparators(ffmpegPath) << "\"\r\n";
	fs << "SET audioStartSecond=" << secStart << "\r\n";
	fs << "SET sourceFile=\"" << QDir::toNativeSeparators(sourceFile) << "\"\r\n";
	fs << "SET destFile=\"" << QDir::toNativeSeparators(targetFile) << "\"\r\n";
	// has to be done in TWO steps, 1. seperate aac, 2. merge aac with video. the reason is ffmpeg has a b/ug that if the source sound duration is hard to detect, it will mess up everything including the video
	fs << "%ffmpegPath% -i %sourceFile% -map 0:a -c copy ~sound.aac\r\n";
	fs << "%ffmpegPath% -ss %audioStartSecond% -i ~sound.aac -i %destFile% -map 0:a -map 1:v -acodec aac -vcodec libxvid -q:v 5 -q:a 3 -filter:v \"setpts=PTS-STARTPTS\" -shortest ~temp_video.mp4\r\n";
	fs << "del ~sound.aac \r\n";
	fs << "move /Y ~temp_video.mp4 %destFile%\r\n";
	fs << "rename %destFile% *.mp4\r\n";
	fs << "@echo " << tr("Now you can delete original video and this file") << "\r\n";
	fs << "pause" << "\r\n";
	f.close();
}
