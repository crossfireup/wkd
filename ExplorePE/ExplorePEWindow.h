#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ExplorePE.h"

class ExplorePEWindow : public QMainWindow
{
	Q_OBJECT

public:
	ExplorePEWindow(QWidget *parent = Q_NULLPTR);

private:
	Ui::ExplorePEClass ui;
};
