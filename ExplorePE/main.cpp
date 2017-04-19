#include "stdafx.h"
#include "ExplorePEWindow.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	ExplorePEWindow w;
	w.show();
	return a.exec();
}
