#include "ui/CheckBoxSelectionDialog.h"


namespace CLC
{
	CheckBoxSelectionDialog::CheckBoxSelectionDialog(const QString& windowTitle, QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
		ui.scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop);
		setWindowTitle(windowTitle);
		hide();
	}

	CheckBoxSelectionDialog::~CheckBoxSelectionDialog()
	{
		qDeleteAll(m_checkBoxes);
		m_checkBoxes.clear();
	}

	void CheckBoxSelectionDialog::setItems(const QVector<Element>& items)
	{
		qDeleteAll(m_checkBoxes);
		m_checkBoxes.clear();
		m_checkBoxes.reserve(items.size());
		for (const auto& item : items)
		{
			QCheckBox* checkBox = new QCheckBox(item.name);
			checkBox->setChecked(item.selected);
			checkBox->setToolTip(item.tooltip);
			m_checkBoxes.append(checkBox);
			ui.scrollAreaWidgetContents->layout()->addWidget(checkBox);
		}
		show();
	}

	void CheckBoxSelectionDialog::on_cancel_pushButton_clicked()
	{
		emit cancelButtonClicked();
		hide();
	}
	void CheckBoxSelectionDialog::on_ok_pushButton_clicked()
	{
		QVector<Element> items;
		items.reserve(m_checkBoxes.size());
		for (int i = 0; i < m_checkBoxes.size(); ++i)
		{
			Element item;
			item.name = m_checkBoxes[i]->text();
			item.selected = m_checkBoxes[i]->isChecked();
			item.tooltip = m_checkBoxes[i]->toolTip();
			items.append(item);
		}
		emit okButtonClicked(items);
		hide();
	}
	void CheckBoxSelectionDialog::closeEvent(QCloseEvent* event)
	{
		QWidget::closeEvent(event);
		emit dialogClosed();
	}
}