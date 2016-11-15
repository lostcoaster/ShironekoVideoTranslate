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

ShironekoTranslate::ShironekoTranslate(QWidget* parent)
	: QMainWindow(parent){
	ui.setupUi(this);

	QObject::connect(ui.startButton, SIGNAL(clicked()), this, SLOT(startPauseVideo()));
	QObject::connect(ui.stopButton, SIGNAL(clicked()), this, SLOT(finish()));
	QObject::connect(&player, SIGNAL(newFrame(QImage, QImage)), this, SLOT(updateImage(QImage, QImage)));
	QObject::connect(&player, SIGNAL(errorReport(QString)), this, SLOT(reportError(QString)));
	QObject::connect(&player, SIGNAL(infoReport(QString)), this, SLOT(showInfos(QString)));
	QObject::connect(ui.tickButton, SIGNAL(clicked()), this, SLOT(startSubstitledVideo()));

//	subtitle.reset(new SubtitleCreator(QSize(ui.resultImageDown->width(), ui.resultImageDown->height() + ui.resultImageUp->height())));

	detector = std::make_shared<MarkDetector>();
	player.addProcessor(detector);

	firstSubtitle = true;
	finishCountDown = -1;

	sourceFile = QFileDialog::getOpenFileName(this, tr("Select Source"));
	destinationFile = QFileDialog::getSaveFileName(this, tr("Select Target"), "", tr("Video File (*.avi)"));
	if(!player.loadVideo(QDir::toNativeSeparators(sourceFile), destinationFile)) {
		reportError(tr("Cannot open file "));
		ui.startButton->setEnabled(false);
		ui.tickButton->setEnabled(false);
	}

	subtitle = std::make_unique<SubtitleCreator>(player.getFrameSize());
//	QObject::connect(subtitle.get(), SIGNAL(newImage(QImage)), this, SLOT(updateBottomImage(QImage)));
	QObject::connect(subtitle.get(), SIGNAL(newImage(QImage)), &player, SLOT(onSubtitleFrameArrive(QImage)), Qt::DirectConnection);
	QObject::connect(&player, SIGNAL(newFrame(QImage, QImage)), subtitle.get(), SLOT(requestNewFrame())); // syncing the frames between player and subtitle
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
	int startFrame = subtitle->getFirstSubtitleStart();
	// select save file
	//	while (destinationFile.isEmpty()) {
	//		destinationFile = QFileDialog::getSaveFileName(this, tr("Select Target"), "", tr("Video File (*.avi)"));
	//		if (destinationFile.isEmpty()) {
	//			QMessageBox box(QMessageBox::Information, tr("Alert"), tr("Really want to discard all changes?"), QMessageBox::Yes | QMessageBox::No);
	//			if(box.exec() == QMessageBox::Yes) {
	//				break;
	//			}
	//		}
	//	}
	if (!destinationFile.isEmpty()) {
		// destroy the signals, although not so necessary
		//		ui.startButton->disconnect();
		//		ui.tickButton->disconnect();
		//		this->disconnect();
		//		player.disconnect();
		//		subtitle->disconnect();
		//		detector->disconnect();
		//
		//		QObject::connect(&player, SIGNAL(errorReport(QString)), this, SLOT(showInfos(QString)));
		//		QObject::connect(&player, SIGNAL(infoReport(QString)), this, SLOT(showInfos(QString)));
		//		QObject::connect(subtitle.get(), SIGNAL(newImage(QImage)), &player, SLOT(onSubtitleFrameArrive(QImage)), Qt::DirectConnection);
		//		// reload video
		//		player.startRecord();
		//		player.loadVideo(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(destinationFile));
		//		subtitle->startBuilder(); // todo use sync'd here
		//		subtitle->restartTrace();
		//		// note that the connection between subtitle and player is still active atm, that's why the following works
		//		QProgressDialog progress(tr("Generating..."), tr("Cancel"), 0, frames);
		//		progress.setWindowModality(Qt::WindowModal);
		//		int numFrame = 0;
		//		do {
		//			++numFrame;
		//			subtitle->advanceTrace(numFrame);
		//			subtitle->requestNewFrame();
		//			player.triggerFrameManually();
		//			qDebug() << "frame " << numFrame << " done";
		//			if (progress.wasCanceled()) {
		//				break;
		//			}
		//			progress.setValue(numFrame);
		//			progress.show();
		//		} while (player.isPlaying() && frames > numFrame);
		FinalizerCreater fc;
		fc.setSourceFile(sourceFile);
		fc.setTargetName(destinationFile);
		fc.setStart(static_cast<double>(startFrame) / player.getFrameRate());
		fc.generateCommand(QFileInfo(destinationFile).dir().absoluteFilePath("AddAudioAndFinalize.bat"));
		QMessageBox box(QMessageBox::Information, tr("Finish"), tr("Output finished."), QMessageBox::Ok);
		box.exec();
		//		subtitle->stopBuilder(-1);
	}
	QApplication::exit();
}


void ShironekoTranslate::reportError(QString error) const {
	QMessageBox box(QMessageBox::Warning, tr("Error"), error, QMessageBox::Ok);
	box.exec();
}
