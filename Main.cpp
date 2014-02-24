#include "DesktopCapture.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// NOTE: this is needed to use QPixmaps inside threads,
	// but only on linux because of X11 architecture.
	app.setAttribute(Qt::AA_X11InitThreads, true);
	app.setQuitOnLastWindowClosed(false);

	DesktopCapture desktopCapture;
	desktopCapture.show();
	return app.exec();
}


