#include "DesktopCapture.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	DesktopCapture desktopCapture;
	desktopCapture.show();
	return app.exec();
}


