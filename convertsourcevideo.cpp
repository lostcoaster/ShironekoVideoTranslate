#include "convertsourcevideo.h"

ConvertSourceVideo::ConvertSourceVideo(QWidget* parent)
	: QDialog(parent) {

	ui.setupUi(this);
	
	frameRate = ui.frameRate->value();
	QObject::connect(ui.frameRate, SIGNAL(valueChanged(double)), this, SLOT(setFrameRate(double)));
}

ConvertSourceVideo::~ConvertSourceVideo() {
}

void ConvertSourceVideo::setFrameRate(double value) {
	frameRate = value;
}
