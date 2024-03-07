#include "ui/MainWindow.h"

namespace CLC
{
	MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent)
	{
		ui.setupUi(this);

		ribbon = new CLC::RibbonImpl(ui.ribbon_widget);
	}

	MainWindow::~MainWindow()
	{
		delete ribbon;
	}
}
