#include "mergeaudioencoder.h"

MergeAudioEncoder::MergeAudioEncoder(QWidget* parent, double audioStartTime, double audioDelayTime, double duration): QDialog(parent) {
	ui.setupUi(this);

	ui.startTime->setValue(audioStartTime);
	ui.duration->setValue(duration);
	ui.delayTime->setValue(audioDelayTime);
}

MergeAudioEncoder::~MergeAudioEncoder() {
}
