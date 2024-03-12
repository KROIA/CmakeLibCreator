#include "ui/CheckBoxSelectionDialog.h"


namespace CLC
{
	CheckBoxSelectionDialog::CheckBoxSelectionDialog(const QString& windowTitle, QWidget* parent)
		: QWidget(parent)
	{
		ui.setupUi(this);
		ui.scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop);
		ui.search_label->setPixmap(QIcon(":/icons/search.png").pixmap(20,20));
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
			// set backgorund color
			
			checkBox->setAutoFillBackground(true);
			checkBox->setStyleSheet("QCheckBox { background-color: " + item.color.name() + "; }");
			
			checkBox->setChecked(item.selected);
			checkBox->setToolTip(item.tooltip);
			m_checkBoxes.append(checkBox);
			ui.scrollAreaWidgetContents->layout()->addWidget(checkBox);
		}
		on_search_lineEdit_textChanged(ui.search_lineEdit->text());
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
			//item.color = QColor(255, 255, 255);
			items.append(item);
		}
		emit okButtonClicked(items);
		hide();
	}
	void CheckBoxSelectionDialog::on_search_lineEdit_textChanged(const QString& text)
	{
		// Filter the list of visible checkboxes
		for (QCheckBox* checkBox : m_checkBoxes)
		{
			checkBox->setVisible(checkBox->text().contains(text, Qt::CaseInsensitive));
		}
	}
	void CheckBoxSelectionDialog::closeEvent(QCloseEvent* event)
	{
		QWidget::closeEvent(event);
		emit dialogClosed();
	}
}