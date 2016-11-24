#pragma once

#include <QtWidgets/QDialog>
#include "ui_convert_codec_dialog.h"

class ConvertSourceVideo : public QDialog
{
	Q_OBJECT

public:
	ConvertSourceVideo(QWidget *parent);
	~ConvertSourceVideo();

	double frameRate;
private:
	Ui::ConvertSourceUI ui;

	private slots:
	void setFrameRate(double);
};
