#pragma once
#include "CmakeLibraryCreator_base.h"
#include <QMainWindow>
#include "ui_MainWindow.h"
#include "RibbonImpl.h"
#include "ProjectSettingsDialog.h"

namespace CLC
{
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	private slots:
		void onOpenTemplatePath_clicked();
		void onDownloadTemplate_clicked();

		void onOpenExistingProject_clicked();
		void onSaveExistingProject_clicked();
		void onSaveAsNewProject_clicked();
	private:
		Ui::MainWindow ui;
		RibbonImpl* m_ribbon;
		ProjectSettingsDialog * m_projectSettingsDialog;
		bool m_existingProjectLoaded = false;
	};
}