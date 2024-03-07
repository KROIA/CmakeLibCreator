#pragma once
#include "CmakeLibraryCreator_base.h"
#include <QMainWindow>
#include "ui_MainWindow.h"
#include "RibbonImpl.h"

namespace CLC
{
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	private:
		Ui::MainWindow ui;
		CLC::RibbonImpl* ribbon;
	};
}