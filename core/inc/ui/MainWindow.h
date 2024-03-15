#pragma once
#include "CmakeLibraryCreator_base.h"
#include <QMainWindow>
#include "ui_MainWindow.h"
#include "RibbonImpl.h"
#include "ProjectSettingsDialog.h"
#include "SettingsDialog.h"

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

		void on_actionVersion_triggered();
		void on_actionAbout_triggered();

		void on_actionSettings_triggered();
		void onExportDialogOkButtonClicked(const QVector<CheckBoxSelectionDialog::Element>& selectedItems);
	private:
		Ui::MainWindow ui;
		RibbonImpl* m_ribbon;
		ProjectSettingsDialog * m_projectSettingsDialog;
		CheckBoxSelectionDialog* m_exportSettingsDialog;
		SettingsDialog * m_settingsDialog;
		bool m_existingProjectLoaded = false;
	};
}
