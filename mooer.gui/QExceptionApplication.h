#pragma once

#include <QApplication>


class QExceptionApplication: public QApplication
{
public:
	QExceptionApplication(int &argc, char ** argv, QString title);
private:
	bool notify(QObject *receiver_, QEvent *event_);

	QString m_title;
};
