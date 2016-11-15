#include <windows.h>
#include "shironekotranslate.h"
#include <QtWidgets/QApplication>
#include "opencv2/opencv.hpp"
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	Q_INIT_RESOURCE(shironekotranslate);

	QTranslator translator;
	translator.load(":/ShironekoTranslate/shironekotranslate_zh");
	a.installTranslator(&translator);

    ShironekoTranslate w;
    w.show();
    return a.exec();
}
