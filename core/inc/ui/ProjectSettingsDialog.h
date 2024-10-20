#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>
#include "ui_ProjectSettingsDialog.h"
#include "ProjectSettings.h"
#include "CheckBoxSelectionDialog.h"
#include <map>

namespace CLC
{

	class ProjectSettingsDialog : public QWidget
	{
		Q_OBJECT

	public:
		ProjectSettingsDialog(QWidget* parent = nullptr);
		~ProjectSettingsDialog();

		void setSettings(const ProjectSettings& settings);
		ProjectSettings getSettings();

		QStringList getCustomDefines() const;
		QStringList getCustomGlobalDefines() const;

	private slots:
		void on_qtModules_pushButton_clicked();
		void on_dependencies_pushButton_clicked();
		void on_libraryName_lineEdit_textChanged(const QString& text);
		void on_qt_useNewestVersion_checkBox_clicked(bool checked);


		void onQtModulesSelected(const QVector<CheckBoxSelectionDialog::Element>& selectedItems);
		void onDependenciesSelected(const QVector<CheckBoxSelectionDialog::Element>& selectedItems);
	private:
		Ui::ProjectSettingsDialog ui;

		ProjectSettings m_settings;

		CheckBoxSelectionDialog *m_qtModulesDialog;
		CheckBoxSelectionDialog *m_dependenciesDialog;

		bool m_ignoreNameChangeEvents = false;
		
		std::map<int, QString> m_qtMajorVersionText;
	};
}
