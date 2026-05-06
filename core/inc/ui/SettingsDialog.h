#pragma once

#include <QWidget>
#include "ui_SettingsDialog.h"

class QVBoxLayout;
class QLineEdit;

namespace CLC
{
	class SettingsDialog : public QWidget
	{
		Q_OBJECT

	public:
		SettingsDialog(QWidget *parent = nullptr);
		~SettingsDialog();


	private slots:
		void on_apply_pushButton_clicked();
		void on_cancel_pushButton_clicked();
		void on_defaultLibraryPathBrowse_toolButton_clicked();
		void on_addProjectPath_pushButton_clicked();
	private:
		void loadSettings();
		void saveSettings();
		void addProjectPathRow(const QString& initial);
		void clearProjectPathRows();
		void showEvent(QShowEvent* event) override;

		// Each project-path row holds its QLineEdit pointer in the row container's `lineEdit` property
		// for retrieval at save time.
		QVBoxLayout* m_projectPathsLayout = nullptr;
		Ui::SettingsDialog ui;
	};
}
