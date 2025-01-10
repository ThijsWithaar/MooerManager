#include "QExceptionApplication.h"

#include <QMessageBox>


QExceptionApplication::QExceptionApplication(int &argc, char ** argv, QString title):
	QApplication(argc, argv),
	m_title(title)
{
}


bool QExceptionApplication::notify(QObject *receiver_, QEvent *event_)
{
	try
	{
		return QApplication::notify(receiver_, event_);
	}
	catch(std::exception& e)
	{
		QMessageBox msg;
		msg.setWindowTitle(m_title);
		msg.setText(QString(e.what()));
		msg.exec();
		return false;
	}
}
