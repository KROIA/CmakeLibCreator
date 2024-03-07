#include "CmakeLibraryCreator_info.h"

#include <iostream>

#if defined(QT_CORE_LIB) and defined(QT_WIDGETS_LIB)
	#include <QWidget>
	#include <QLabel>
	#include <QVBoxLayout>
	#include <vector>
	#define QT_WIDGETS_AVAILABLE
#endif

namespace CLC
{
	void LibraryInfo::printInfo()
	{
		printInfo(std::cout);
	}
	void LibraryInfo::printInfo(std::ostream& stream)
	{
		std::stringstream ss;
		ss << "Library Name: " << name << "\n"
			<< "Author: " << author << "\n"
			<< "Email: " << email << "\n"
			<< "Website: " << website << "\n"
			<< "License: " << license << "\n"
			<< "Version: " << versionStr() << "\n"
			<< "Compilation Date: " << compilationDate << "\n"
			<< "Compilation Time: " << compilationTime << "\n";

		stream << ss.str();
	}

#ifdef QT_WIDGETS_AVAILABLE
	void addRow(const QString& labelText, const QString& valueText, QGridLayout* layout, int row) {
		QLabel* label = new QLabel(labelText);
		label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		layout->addWidget(label, row, 0);

		QLabel* valueLabel = new QLabel(valueText);
		layout->addWidget(valueLabel, row, 1);
	}
	QWidget* LibraryInfo::createInfoWidget(QWidget* parent)
	{
		QWidget* widget = new QWidget(parent);
		QGridLayout* layout = new QGridLayout(widget);
		
		struct Pair 
		{
			std::string label;
			std::string value;
		};
		std::vector< Pair> pairs = {
			{"Library Name:", name},
			{"Author:", author},
			{"Email:", email},
			{"Website:", website},
			{"License:", license},
			{"Version:", versionStr()},
			{"Compilation Date:", compilationDate},
			{"Compilation Time:", compilationTime},
			{"Build Type:", buildTypeStr},
		};
		int rowCount = 0;
		for (const auto& pair : pairs) {
			addRow(QString::fromStdString(pair.label), QString::fromStdString(pair.value), layout, rowCount++);
		}

		widget->setLayout(layout);
		return widget;
	}
#else
	QWidget* LibraryInfo::createInfoWidget(QWidget* parent)
	{
		return nullptr;
	}
#endif
}