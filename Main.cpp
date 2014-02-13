#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainWindow desktopCapture;
	desktopCapture.show();
	return app.exec();
}


