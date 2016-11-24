#include "shironekotranslate.h"
#include <QPainter>
#include <QImage>
#include <QInputDialog>
#include <QFileDialog>
#include <QScrollBar>
#include <QProgressDialog>
#include <QMessageBox>
#include <QtDebug>
#include "finalizercreater.h"
#include <QApplication>
#include "ffmpegdirector.h"
#include "convertsourcevideo.h"
#include "mergeaudioencoder.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 1
#define VERSION_BUILD 0

ShironekoTranslate::ShironekoTranslate(QWidget* parent)
	: QMainWindow(parent), player(this){
	ui.setupUi(this);
	setWindowTitle(tr("Translate") + " " +
		QString::number(VERSION_MAJOR) + "." +
		QString::number(VERSION_MINOR) + "." +
		QString::number(VERSION_BUILD) + " - by lc"
	);
}

void ShironekoTranslate::initialize() {

	detector = std::make_shared<MarkDetector>(this);
	player.addProcessor(detector);

	firstSubtitle = true;
	finishCountDown = -1;

	selectAndLoadVideo();

	subtitle = std::make_unique<SubtitleCreator>(this, player.getFrameSize());

	initConnections();
}

void ShironekoTranslate::updateImage(QImage full, QImage upper) {
	ui.imageLabel->setPixmap(QPixmap::fromImage(full));
	ui.resultImage->setPixmap(QPixmap::fromImage(upper));
	if (finishCountDown > 0) {
		--finishCountDown;
		if (finishCountDown == 0) {
			finishVideo();
		}
	}
}

//void ShironekoTranslate::updateBottomImage(QImage img) {
//	ui.resultImageDown->setPixmap(QPixmap::fromImage(img).scaled(ui.resultImageDown->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
//	emit subtitleChanged(img);
//}

void ShironekoTranslate::showInfos(QString info) const {
//	ui.statusBar->showMessage(info, 4000);
}

void ShironekoTranslate::startPauseVideo() {
	if (player.isPlaying()) {
		player.pauseVideo();
		ui.startButton->setText(tr("Start"));
	} else {
		player.playVideo();
		ui.startButton->setText(tr("Pause"));
		if(!ui.stopButton->isEnabled()) {
			ui.stopButton->setEnabled(true);
		}
	}
}

void ShironekoTranslate::startSubstitledVideo() {
	if (firstSubtitle) {
//		ui.tickButton->setStyleSheet("background: green;");
//		ui.tickButton->setText(tr("Mark Stop"));
		firstSubtitle = false;
		ui.tickButton->setEnabled(false);
		ui.tickButton->hide();

		QObject::connect(detector.get(), SIGNAL(subtitleStart()), this, SLOT(startSubtitle()));
		QObject::connect(detector.get(), SIGNAL(subtitleStop()), this, SLOT(stopLastSubtitle()));

		detector->initialize();
	}
}

void ShironekoTranslate::startSubtitle() {
	QString data;
	QString text = ui.comingTextPreview->toPlainText();
	while (data.isEmpty()) {
		if (text.isEmpty()) {
			return; // nothing to add
		}

		int i = text.indexOf('\n');
		data = text.left(i).trimmed();
		if (i >= 0) {
			text = text.mid(i + 1);
		}
		else {
			text = "";
		}
	}
	ui.comingTextPreview->setPlainText(text);
	ui.doneTextPreview->insertPlainText(data + '\n');
	auto * b = ui.doneTextPreview->verticalScrollBar();
	b->setValue(b->maximum());
	subtitle->addText(data, player.getFrameNumber());
}

void ShironekoTranslate::stopLastSubtitle() {
	subtitle->endText(player.getFrameNumber());
}

void ShironekoTranslate::finish() {
	if (player.isPlaying()) {
		subtitle->skipAnimation();
		finishCountDown = 3;
	} else {
		finishVideo();
	}
}


void ShironekoTranslate::finishVideo() {
	// finished , now rebuild the video
	player.pauseVideo();
	if (firstSubtitle) {
		return; // nothing to save.
	}
	int frames = player.getFrameNumber();
	detector->finish();
	subtitle->stopBuilder(frames - 1); // just try to skip animation more reliable
	player.releaseAll();
	int startFrame = subtitle->getFirstSubtitleStart();
	if (!destinationFile.isEmpty()) {
//		FinalizerCreater fc(this);
//		fc.setSourceFile(sourceFile);
//		fc.setTargetName(destinationFile);
//		fc.setStart(static_cast<double>(startFrame) / player.getFrameRate());
//		fc.generateCommand(QFileInfo(destinationFile).dir().absoluteFilePath("AddAudioAndFinalize.bat"));
//		QMessageBox box(QMessageBox::Information, tr("Finish"), tr("Output finished."), QMessageBox::Ok);
//		box.exec();
		MergeAudioEncoder m(this, static_cast<double>(startFrame) / player.getFrameRate(), 0.0, static_cast<double>(frames-startFrame+1) / player.getFrameRate());
		while(!m.exec()) {
			QMessageBox box(QMessageBox::Warning, tr("Exit"), tr("Confirm discard all work?"), QMessageBox::Ok | QMessageBox::Cancel);
			if(box.exec()) {
				emit quitApplication();
			}
		}
		FfmpegDirector f(this, "ffmpeg.exe");
		QProgressDialog p(tr("Merging"), QString(), 0, 100, this);
		p.setWindowModality(Qt::WindowModal);
		QObject::connect(&f, SIGNAL(updateProgress(int)), &p, SLOT(setValue(int)));

		QMap<QString, double> parameters;
		parameters["duration"] = m.getDuration();
		parameters["audioStart"] = m.getStartTime() + m.getDelayTime();
		f.addAudioAndEncode(sourceFile, destinationFile, parameters);
		p.show();
		while (!f.isFinished()) {
			f.waitForFinish(30000);
		}
		QDir().remove(destinationFile); // we have a .mp4 file now
		QMessageBox box(QMessageBox::Warning, tr("Exit"), tr("Finish! You can delete now."), QMessageBox::Ok);
		box.exec();
	}
	QApplication::exit();
}

bool ShironekoTranslate::fixVariableFps() {
	ConvertSourceVideo c(nullptr); // note that we can't use parent here, as the mainform's event loop is not started yet
//	c.setWindowModality(Qt::WindowModal);
	bool ret = c.exec();
	if(!ret) {
		reportError(tr("Aborted conversion"));
	}
	FfmpegDirector f(this, "ffmpeg.exe");
	QProgressDialog p(tr("Fixing"), QString(), 0, 100, this);
	p.setWindowModality(Qt::WindowModal);
	QObject::connect(&f, SIGNAL(updateProgress(int)), &p, SLOT(setValue(int)));

	QString tempFile = QFileInfo(sourceFile).dir().absoluteFilePath("ori_" + QFileInfo(sourceFile).fileName());
	ret = QDir().rename(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(tempFile));
	f.convertToFixedFrameRate(tempFile, c.frameRate, sourceFile);
	p.show();
	while(!f.isFinished()) {
		f.waitForFinish(30000);
	}
	QDir().remove(tempFile);
	return true;
}

void ShironekoTranslate::initConnections() {
	QObject::connect(ui.startButton, SIGNAL(clicked()), this, SLOT(startPauseVideo()));
	QObject::connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(finish()));
	QObject::connect(&player, SIGNAL(newFrame(QImage, QImage)), this, SLOT(updateImage(QImage, QImage)));
	QObject::connect(&player, SIGNAL(errorReport(QString)), this, SLOT(reportError(QString)));
	QObject::connect(&player, SIGNAL(infoReport(QString)), this, SLOT(showInfos(QString)));
	QObject::connect(ui.tickButton, SIGNAL(clicked()), this, SLOT(startSubstitledVideo()));

	//	QObject::connect(subtitle.get(), SIGNAL(newImage(QImage)), this, SLOT(updateBottomImage(QImage)));
	QObject::connect(subtitle.get(), SIGNAL(newImage(QImage)), &player, SLOT(onSubtitleFrameArrive(QImage)), Qt::DirectConnection);
	QObject::connect(&player, SIGNAL(newFrame(QImage, QImage)), subtitle.get(), SLOT(requestNewFrame())); // syncing the frames between player and subtitle
}

void ShironekoTranslate::selectAndLoadVideo() {
	sourceFile = QFileDialog::getOpenFileName(this, tr("Select Source"));
	destinationFile = QFileDialog::getSaveFileName(this, tr("Select Target"), "", tr("Video File (*.avi)"));

	if (QDir().exists(destinationFile.left(destinationFile.lastIndexOf('.')) + ".mp4")) {
		// the final file is mp4, so we have to check it too
		reportError(tr("The final file is mp4, which exists"));
	}
	auto state = player.loadVideo(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(destinationFile));

	if (state != ShironekoVideoPlayer::Success) {
		if (state == ShironekoVideoPlayer::VaribalFPS) {
			fixVariableFps();
			state = player.loadVideo(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(destinationFile));
			if (state != ShironekoVideoPlayer::Success) {
				reportError(tr("Fix failed"));
			}
		}
		else {
			switch (state) {
			case ShironekoVideoPlayer::NoDecoder:
				reportError(tr("Can't use this source file"));
				break;
			case ShironekoVideoPlayer::NoEncoder:
				reportError(tr("No encoder!"));
				break;
			case ShironekoVideoPlayer::WriteError:
				reportError(tr("Can't write to file"));
				break;
			default:
				reportError(tr("Unknow error, wait.."));
			}
		}
	}
}


void ShironekoTranslate::reportError(QString error) {
	QMessageBox box(QMessageBox::Warning, tr("Error"), error, QMessageBox::Ok);
	box.exec();

	QApplication::exit(-1);
}
