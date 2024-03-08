#pragma once

#include <QWidget>
#include "ui_CheckBoxSelectionDialog.h"
#include <QCheckBox>

namespace CLC
{
	class CheckBoxSelectionDialog : public QWidget
	{
		Q_OBJECT

	public:
		struct Element
		{
			QString name;
			bool selected;
			QString tooltip;
		};
		CheckBoxSelectionDialog(const QString &windowTitle, QWidget *parent = nullptr);
		~CheckBoxSelectionDialog();

		void setItems(const QVector<Element> &items);
	signals:
		void okButtonClicked(const QVector<Element> &selectedItems);
		void cancelButtonClicked();
		void dialogClosed();
	private slots:
		void on_cancel_pushButton_clicked();
		void on_ok_pushButton_clicked();

		
	private:
		void closeEvent(QCloseEvent *event) override;

		Ui::CheckBoxSelectionDialog ui;
		QVector<QCheckBox*> m_checkBoxes;
	};
}