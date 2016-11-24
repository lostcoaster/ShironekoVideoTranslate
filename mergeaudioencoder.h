#pragma once

#include <QWidget>
#include "ui_output_setting.h"

class MergeAudioEncoder : public QDialog
{
	Q_OBJECT

public:
	explicit MergeAudioEncoder(QWidget *parent, double audioStartTime, double audioDelayTime, double duration);
	~MergeAudioEncoder();

	double getStartTime() const { return ui.startTime->value(); }
	double getDelayTime() const { return ui.delayTime->value(); }
	double getDuration() const { return ui.duration->value(); }
private:
	Ui::OutputSettingUI ui;
};
