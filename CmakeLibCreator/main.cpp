#include <QApplication>
#include <iostream>
#include "CmakeLibraryCreator.h"

#include <QWidget>
#include <QFile>


int main(int argc, char* argv[])
{
	
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	QApplication app(argc, argv);

	// Set QT stylesheet from file:
	

	CLC::MainWindow mainWindow;
	mainWindow.show();
	//CLC::LibraryInfo::createInfoWidget()->show();
	
	return app.exec();
}