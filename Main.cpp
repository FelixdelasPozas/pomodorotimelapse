#include "MainWindow.h"

#include <QApplication>
#include "Screenshot.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	MainWindow screenshot;
	screenshot.show();
	return app.exec();
}


