#include "Dependency.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

namespace CLC
{
	Dependency::Dependency()
	{

	}
	Dependency::Dependency(const Dependency& other)
		: m_name(other.m_name)
		, m_description(other.m_description)
	{

	}
	Dependency::Dependency(const QString& name, const QString& description)
		: m_name(name)
		, m_description(description)
	{

	}
	Dependency::~Dependency()
	{

	}

	Dependency& Dependency::operator=(const Dependency& other)
	{
		m_name = other.m_name;
		m_description = other.m_description;
		return *this;
	}
	bool Dependency::operator==(const Dependency& other) const
	{
		if (m_name.length() != other.m_name.length() ||
			m_description.length() != other.m_description.length())
			return false;
		return	m_name == other.m_name &&
			m_description == other.m_description;
	}
	bool Dependency::operator!=(const Dependency& other) const
	{
		if (m_name.length() != other.m_name.length() ||
			m_description.length() != other.m_description.length())
			return true;
		return	m_name != other.m_name ||
			m_description != other.m_description;
	}

	void Dependency::setName(const QString& name)
	{
		m_name = name;
	}
	const QString& Dependency::getName() const
	{
		return m_name;
	}

	void Dependency::setDescription(const QString& description)
	{
		m_description = description;
	}
	const QString& Dependency::getDescription() const
	{
		return m_description;
	}
	bool Dependency::loadFromCmakeFile(const QString& filePath)
	{
		// Read file
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qDebug() << "Failed to open file: " << filePath;
			return false;
		}
		// Read file name
		m_name = file.fileName().split("/").last().split(".").first();
		// read lines
		QTextStream in(&file);
		QString line;
		m_description = "No description available";
		bool descriptionFound = false;
		while (!in.atEnd())
		{
			line = in.readLine();
			if (line.contains("#"))
			{
				line = line.mid(line.indexOf("#"));
				if (line.toLower().contains("description:"))
				{
					m_description = line.mid(line.indexOf(":") + 1).trimmed();
					descriptionFound = true;
					file.close();
					return descriptionFound;
				}
			}
		}
		return descriptionFound;
	}

}