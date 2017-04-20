#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ExplorePE.h"

class ExplorePEWindow : public QMainWindow
{
	Q_OBJECT

public:
	ExplorePEWindow(QWidget *parent = Q_NULLPTR);

	private slots:
	int chooseProcess();
	void showAboutInfo();

private:
	Ui::ExplorePEClass ui;
	
	void createMenus();
		
};