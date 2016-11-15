#pragma once
#include <QObject>
#include <QImage>
#include <QFont>
#include <QMutex>
#include <qfuture.h>
#include <qwaitcondition.h>

//todo
//high priority: stopping animate when user requests or quitting. wait for concurrency. output subtitle timeline file
//low priority: dialog background.

class SubtitleCreator :
	public QObject {
	Q_OBJECT
public:
	SubtitleCreator(QSize mainFrameSize);
	~SubtitleCreator();
	void addText(QString content, int framePos);
	void endText(int framePos);
	void restartTrace();
	void advanceTrace(int frameNumber);
	void startBuilder();
	void stopBuilder(int frameNumber);
	int getFirstSubtitleStart();
	void skipAnimation();
private:
	int textWidthPerLine;
	QRect titleRect;
	int topMargin;
	int boxLeftMargin;
	int boxWidth;
	int betweenBoxMargin;
	int contentLeft;
	int boxTextHeight;
	int backgroundTextHeight;
	QList<QImage> backgroundBox;
	QList<QImage> box;
	void estimateSizes(const QSize& size);


//	QRect canvas;
	QImage background;
	QImage drawn;
	QList<QFont> fonts;
	QList<qreal> fontHeights;
	QList<QColor> colors;

	QWaitCondition frameQuotaCondition; // use to pause threads
	QWaitCondition frameFullySpentCondition; // used for synced frame
	QMutex accessQuotaMutex;
	int frameQuota; // how many frames subtitle should but not rendered
//	QWaitCondition pendingSubtitlesCondition;
//	QMutex accessTraceMutex; // access playingTrace
	QFuture<void> builderRunning;
	bool stop;
	bool replaying;

	enum LineType {
		TITLE = 0,
		NORMAL,
		BACKGROUND
	};

	QList<QImage> toDraw;
	int totalLines;
	bool isBackground;
	QList<QImage> subtitlesInShow;
	QImage renderBox(int targetLine, bool lineSpaceFalling, double transitPosition = 1);

	struct SubtitleTrace {
		int framePos;
		int stopPos;
		QString line;
	};
	QList<SubtitleTrace> recordingTrace, playingTrace;
	static const int MAX_FRAME_DURATION = 36;

	void separateLine(QString line);
	QImage paintText(QString line, LineType type);
	void animatedInsert(int frameDuration);
	bool getDrawingFrameQuota();
//	void lineFallEffect(decltype(subtitlesInShow.begin()) it, int frameDuration, int distance);
//	void lineAppearEffect(decltype(subtitlesInShow.begin()) it, int frameDuration);
	void renderCurrentImage(); // render and emit
	void builderExec();

	signals :
	void newImage(QImage);

	public slots:
	void requestNewFrame();
};
