#include "MainWindow.h"

#include <QApplication>

#include "screenshot.h"

using namespace std;
using namespace cv;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	Screenshot screenshot;
	screenshot.show();
	return app.exec();
}

