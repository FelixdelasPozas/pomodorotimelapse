/*
 * Screenshot.h
 *
 *  Created on: 20/12/2013
 *      Author: Felix de las Pozas Alvarez
 */
#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

#include <QWidget>

class QLabel;
class QGroupBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QHBoxLayout;
class QVBoxLayout;
class QGridLayout;

class Screenshot
: public QWidget
{
	Q_OBJECT

	public:
		Screenshot();
		~Screenshot();

	protected:
		void resizeEvent(QResizeEvent *event);

	private slots:
		void newScreenshot();
		void saveScreenshot();
		void shootScreen();
		void updateCheckBox();

	private:
		void createOptionsGroupBox();
		void createButtonsLayout();
		QPushButton *createButton(const QString &text, QWidget *receiver,
				const char *member);
		void updateScreenshotLabel();

		QPixmap originalPixmap;

		QLabel *screenshotLabel;
		QGroupBox *optionsGroupBox;
		QSpinBox *delaySpinBox;
		QLabel *delaySpinBoxLabel;
		QCheckBox *hideThisWindowCheckBox;
		QPushButton *newScreenshotButton;
		QPushButton *saveScreenshotButton;
		QPushButton *quitScreenshotButton;

		QVBoxLayout *mainLayout;
		QGridLayout *optionsGroupBoxLayout;
		QHBoxLayout *buttonsLayout;
};

#endif /* SCREENSHOT_H_ */
