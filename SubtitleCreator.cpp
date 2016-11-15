#include "SubtitleCreator.h"
#include <QPainter>
#include <QtConcurrent/QtConcurrent>
#include <iostream>

SubtitleCreator::SubtitleCreator(QSize mainFrameSize) {
	fonts.push_back(QFont("LiSu", 28, 99)); // title
	colors.push_back(Qt::white);
	fonts.push_back(QFont("Microsoft Yahei", 24, 60)); // normal
	colors.push_back(QColor(75, 34, 5));
	fonts.push_back(QFont("Microsoft Yahei", 24, 60)); // background
	colors.push_back(Qt::white);

	frameQuota = 0;
	replaying = false;

	stop = false;
	estimateSizes(mainFrameSize);
	startBuilder();
}

SubtitleCreator::~SubtitleCreator() {
	stopBuilder(-1);
}

void SubtitleCreator::addText(QString content, int framePos) {
	playingTrace.push_back(SubtitleTrace{framePos, -1, content});
}

void SubtitleCreator::endText(int framePos) {
	if (!recordingTrace.isEmpty() && framePos < recordingTrace.back().stopPos) {
		skipAnimation();
		recordingTrace.back().stopPos = framePos;
	} // otherwise the given duration is too long we have shown everything
}

void SubtitleCreator::builderExec() {
	forever {
		accessQuotaMutex.lock();
		if (!stop) {
			frameQuotaCondition.wait(&accessQuotaMutex); // here we wait for 1 frame
		}
		accessQuotaMutex.unlock();
		if (stop) {
			break;
		}

		// if empty , we just move to next frame
		if (!playingTrace.isEmpty()) {
			SubtitleTrace trace = playingTrace.front();

			qDebug() << "Start handling " << trace.line;

			playingTrace.pop_front();

			accessQuotaMutex.lock();
			frameQuota = 0;
			accessQuotaMutex.unlock();

			separateLine(trace.line);
			trace.stopPos = trace.framePos + MAX_FRAME_DURATION * totalLines;
			animatedInsert(trace.stopPos - trace.framePos);
			if (!replaying) {
				// avoid circular insertion when replaying record
				recordingTrace.push_back(trace);
			}
		}
		accessQuotaMutex.lock();
		frameQuota = 0;
		accessQuotaMutex.unlock();
		frameFullySpentCondition.wakeAll();
	}
}

void SubtitleCreator::startBuilder() {
	if (!builderRunning.isRunning()) {
		stop = false;
		playingTrace.clear();
		subtitlesInShow.clear();
		toDraw.clear();
		builderRunning = QtConcurrent::run(std::bind(&SubtitleCreator::builderExec, this));
	}
}

void SubtitleCreator::stopBuilder(int frameNumber) {
	if (builderRunning.isRunning()) {
		stop = true;
		skipAnimation(); // skip animation
		builderRunning.waitForFinished();
		endText(frameNumber);
	}
}

int SubtitleCreator::getFirstSubtitleStart() {
	if (!recordingTrace.isEmpty()) {
		return recordingTrace.front().framePos;
	} else {
		return 0;
	}
}

void SubtitleCreator::estimateSizes(const QSize& size) {
	double ratio = size.height() / 1334.0;
	background.load(":/ShironekoTranslate/Resources/background.png");
	background = background.scaled(size.width(), size.height() * 0.55, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	boxWidth = 671 * ratio;
	topMargin = 47 * ratio;
	boxLeftMargin = (size.width() - boxWidth) / 2;
	titleRect = QRect(20 * ratio, 24 * ratio, 200 * ratio, 34 * ratio);
	contentLeft = 40 * ratio;
	betweenBoxMargin = 50 * ratio;
	double bodyFontSize = 24.2 * ratio;
	double titleFontSize = 25.5 * ratio;

	// fonts
	QFontMetrics fm(fonts[0]);
	fonts[0].setPointSizeF(titleFontSize); // the default title font
	fontHeights.push_back(fm.height());
	fonts[1].setPointSizeF(bodyFontSize); // box
	fm = QFontMetrics(fonts[1]);
	fontHeights.push_back(fm.height());
	fonts[2].setPointSizeF(bodyFontSize); // background
	fm = QFontMetrics(fonts[2]);
	fontHeights.push_back(fm.height());

	textWidthPerLine = boxWidth - 2 * contentLeft;
	box.clear();
	box.push_back(QImage(":/ShironekoTranslate/Resources/box_title.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	box.push_back(QImage(":/ShironekoTranslate/Resources/box_mid.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	box.push_back(QImage(":/ShironekoTranslate/Resources/box_bottom.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	backgroundBox.clear();
	backgroundBox.push_back(QImage(":/ShironekoTranslate/Resources/background_top.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	backgroundBox.push_back(QImage(":/ShironekoTranslate/Resources/background_mid.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	backgroundBox.push_back(QImage(":/ShironekoTranslate/Resources/background_bottom.png").scaled(boxWidth, boxWidth, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SubtitleCreator::restartTrace() {
	replaying = true;
}

void SubtitleCreator::advanceTrace(int frameNumber) {
	// check list
	if (!recordingTrace.isEmpty() && recordingTrace.front().framePos <= frameNumber) {
		if (recordingTrace.front().line.isEmpty()) {
			// a frame just to "skip animation"
			skipAnimation();
		} else {
			// condition here cannot be >=, as due to 
			qDebug() << "adding trace @" << frameNumber << " " << recordingTrace.front().line;
			addText(recordingTrace.front().line, frameNumber); // note the order
			recordingTrace.pop_front();
		}
	}
	qDebug() << "advanced trace to" << frameNumber;
}

void SubtitleCreator::animatedInsert(int frameDuration) {
	int margin = 20; // margin between different subtitles
	if (!toDraw.isEmpty()) {
		subtitlesInShow.push_front(QImage()); // new lines inserting pointer
	}

	bool drawThisFrame = getDrawingFrameQuota();
	int framesPerLine = frameDuration / totalLines;
	int fallFrames = framesPerLine / 3;
	int appearFrames = framesPerLine - fallFrames;
	for (int l_no = 0; l_no < totalLines; ++l_no) {
		// line fall frames
		for (int i = 0; i < fallFrames; ++i) {
			drawThisFrame = getDrawingFrameQuota();
			if (drawThisFrame) {
				subtitlesInShow[0] = renderBox(l_no, true, 1.1 / (fallFrames - 1) * i);
				renderCurrentImage();
			}
		}
		// appear frames
		for (int i = 0; i < appearFrames; ++i) {
			drawThisFrame = getDrawingFrameQuota();
			// we should at least draw the last frame
			if (i == appearFrames - 1 || drawThisFrame) {
				subtitlesInShow[0] = renderBox(l_no, false, 1.1 / (appearFrames - 1) * i);
				renderCurrentImage();
			}
		}
	}

	// finishes and clear the pending list
	toDraw.clear();
}

bool SubtitleCreator::getDrawingFrameQuota() {
	bool ret = false;
	accessQuotaMutex.lock();
	if (frameQuota == 1) {
		ret = true; // skip rendering frames to catch up
	} else if (frameQuota == 0) {
		frameFullySpentCondition.wakeAll();
		frameQuotaCondition.wait(&accessQuotaMutex);
		ret = true;
	} else {
		--frameQuota; // not the last quota, skipping
		ret = false;
	}
	accessQuotaMutex.unlock();
	return ret;
}

//void SubtitleCreator::lineFallEffect(decltype(subtitlesInShow.begin()) it, int frameDuration, int distance) {
//	int fellDistance = 0;
//	for (int i = 0; i < frameDuration; ++i) {
//		auto cit = it;
//		int step = (distance - fellDistance) / (frameDuration - i); // rounding error happends here
//		fellDistance += step;
//		while (cit != subtitlesInShow.end()) {
//			cit->topPos += step;
//			++cit;
//		}
//
//		if (!getDrawingFrameQuota()) {
//			continue; // skip
//		}
//
//		renderCurrentImage();
//	}
//}

//void SubtitleCreator::lineAppearEffect(decltype(subtitlesInShow.begin()) it, int frameDuration) {
//	QImage originalText = it->img.copy();
//	QLinearGradient alphaGradient;
//	QPainter p;
//	alphaGradient.setColorAt(0, Qt::white);
//	alphaGradient.setColorAt(1, Qt::transparent);
//	int totalLength = originalText.width();
//	int shownLength = 0;
//	for (int i = 0; i < frameDuration - 1; ++i) {
//		int start = shownLength;
//		int end = shownLength + (totalLength - shownLength) / (frameDuration - i);
//		shownLength = end;
//
//		if (!getDrawingFrameQuota()) {
//			continue; // skip
//		}
//
//		alphaGradient.setStart(start, 0);
//		alphaGradient.setFinalStop(end, 0);
//
//		it->img = originalText.copy();
//		p.begin(&(it->img));
//		p.setBrush(alphaGradient);
//		p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
//		p.drawRect(it->img.rect());
//		p.end();
//		renderCurrentImage();
//	}
//	it->img = originalText;
//}

void SubtitleCreator::renderCurrentImage() {
	drawn = background.copy();
	int bottom = drawn.height();

	QPainter mainPainter(&drawn);
	mainPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	mainPainter.setRenderHint(QPainter::SmoothPixmapTransform);

	int top = topMargin;
	int i;
	for (i = 0; i < subtitlesInShow.length(); ++i) {
		if (top >= bottom) {
			// remove the rest
			break;
		}
		mainPainter.drawImage(QPoint(boxLeftMargin, top), subtitlesInShow[i]);
		top += subtitlesInShow[i].height() + betweenBoxMargin;
	}

	while (subtitlesInShow.length() > i) {
		subtitlesInShow.pop_back(); // remove
	}


	emit newImage(drawn);
	if (replaying) qDebug() << "Sub frame outputed";
	accessQuotaMutex.lock();
	--frameQuota;
	accessQuotaMutex.unlock();
	frameFullySpentCondition.wakeAll();
}

void SubtitleCreator::skipAnimation() {
	accessQuotaMutex.lock();
	frameQuota = 99999; // should be enough for animation
	accessQuotaMutex.unlock();
	frameQuotaCondition.wakeAll();
}

void SubtitleCreator::requestNewFrame() {
	accessQuotaMutex.lock();
	++frameQuota;
	accessQuotaMutex.unlock();
	frameQuotaCondition.wakeAll();
	if (replaying) {
		// we use sync scheme if replaying
		accessQuotaMutex.lock();
		if (frameQuota != 0) {
			frameFullySpentCondition.wait(&accessQuotaMutex);
		}
		accessQuotaMutex.unlock();
	}
}

QImage SubtitleCreator::renderBox(int targetLine, bool lineSpaceFalling, double transitPosition) {
	decltype(box)& boxPicture = isBackground ? backgroundBox : box;
	QImage body;
	double lineHeight = static_cast<double>(toDraw[1].height()) / totalLines;
	if (lineSpaceFalling) {
		body = boxPicture[1].scaled(boxWidth, lineHeight * (targetLine + transitPosition));
	} else {
		body = boxPicture[1].scaled(boxWidth, lineHeight * (targetLine + 1));
	}
	QImage canvas(boxWidth, boxPicture[0].height() + body.height() + boxPicture[2].height(), QImage::Format_ARGB32_Premultiplied);
	canvas.fill(Qt::transparent);
	QPainter painter(&canvas);
	painter.drawImage(QPoint(0, 0), boxPicture[0]);
	if (!isBackground) {
		// draw title text
		painter.drawImage(QRect(titleRect.left(), titleRect.top(),
		                        std::min(toDraw[0].width(), titleRect.width()),
		                        std::min(toDraw[0].height(), titleRect.height())), toDraw[0]);
	}
	int top = boxPicture[0].height();
	painter.drawImage(QPoint(0, top), body);
	// from the 2nd image, as the first is title
	for (auto i = 0; i < targetLine; ++i) {
		painter.drawImage(QPoint(contentLeft, top), toDraw[1], QRect(0, i * lineHeight, toDraw[1].width(), lineHeight));
		top += lineHeight;
	}
	if (!lineSpaceFalling) {
		// draw the last line
		QImage temp = toDraw[1].copy(QRect(0, targetLine * lineHeight, toDraw[1].width(), lineHeight));
		if (transitPosition < 1) {
			int fadeleft = temp.width() * transitPosition;
			QLinearGradient alphaGradient(QPoint(fadeleft, 0), QPoint(fadeleft + 40, 0)); // 40 pixel fading
			alphaGradient.setColorAt(0, Qt::white);
			alphaGradient.setColorAt(1, Qt::transparent);
			QPainter p(&temp);
			p.setBrush(alphaGradient);
			p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
			p.drawRect(temp.rect());
			p.end();

		}
		painter.drawImage(QPoint(contentLeft, top), temp);
	}
	painter.drawImage(QPoint(0, boxPicture[0].height() + body.height()), boxPicture[2]); // bottom
	painter.end();
	return canvas;
}

void SubtitleCreator::separateLine(QString line) {
	toDraw.clear();
	LineType baseType = NORMAL;
	if (!line.startsWith(QChar('-'))) {
		isBackground = false;
		auto i = line.indexOf(QChar(':'));
		if (i >= 0 && i < 20) {
			toDraw.push_back(paintText(line.left(i).trimmed(), TITLE));
			line = line.mid(i + 1);
		} else {
			// no title
			toDraw.push_back(paintText(" ", TITLE));
		}
	} else {
		line = line.mid(1);
		toDraw.push_back(QImage()); // dummy null img
		isBackground = true;
	}

	//	for (auto i = 0; i < line.length(); i += textWidthPerLine) {
	//		toDraw.push_back(paintText(line.mid(i, textWidthPerLine).trimmed(), isBackground ? LineType::BACKGROUND : LineType::NORMAL));
	//	}
	toDraw.push_back(paintText(line.trimmed(), isBackground ? LineType::BACKGROUND : LineType::NORMAL));
	totalLines = round(static_cast<double>(toDraw.back().height()) / (isBackground ? fontHeights[BACKGROUND] : fontHeights[NORMAL])); //estimate lines, some margins left
}

QImage SubtitleCreator::paintText(QString line, LineType type) {
	QImage textCanvas(QSize(textWidthPerLine, 600), QImage::Format_ARGB32_Premultiplied); // should be large enough 
	textCanvas.fill(Qt::transparent);
	QPainter textPainter(&textCanvas);

	if (type == TITLE && line.length() > 8) {
		// max 8 chars , if more, needs resizing
		QFont newFont = fonts[TITLE];
		newFont.setPointSizeF(newFont.pointSizeF() / line.length() * 8);
		textPainter.setFont(newFont);
	} else {
		textPainter.setFont(fonts[type]);
	}
	QRect bounding; // cropping the image to leave only bounding
	textPainter.setPen(colors[type]);
	textPainter.drawText(textCanvas.rect(), Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere, line, &bounding);
	return textCanvas.copy(bounding);
}
