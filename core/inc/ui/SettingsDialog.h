#pragma once

#include <QWidget>
#include "ui_SettingsDialog.h"

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
	private:
		void loadSettings();
		void saveSettings();	
		void showEvent(QShowEvent* event) override;
		Ui::SettingsDialog ui;
	};
}