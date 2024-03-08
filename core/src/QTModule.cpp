#include "QTModule.h"

namespace CLC
{
	QTModule::QTModule()
	{

	}
	QTModule::QTModule(const QTModule& other)
		: m_name(other.m_name)
		, m_description(other.m_description)
	{

	}
	QTModule::QTModule(const QString name, const QString description)
		: m_name(name)
		, m_description(description)
	{

	}
	QTModule::~QTModule()
	{

	}

	QTModule& QTModule::operator=(const QTModule& other)
	{
		m_name = other.m_name;
		m_description = other.m_description;
		return *this;
	}
	bool QTModule::operator==(const QTModule& other) const
	{
		if (m_name.length() != other.m_name.length() ||
			m_description.length() != other.m_description.length())
			return false;
		return	m_name == other.m_name && 
				m_description == other.m_description;
	}
	bool QTModule::operator!=(const QTModule& other) const
	{
		if (m_name.length() != other.m_name.length() ||
			m_description.length() != other.m_description.length())
			return true;
		return	m_name != other.m_name || 
				m_description != other.m_description;
	}

	void QTModule::setName(const QString& name)
	{
		m_name = name;
	}
	const QString& QTModule::getName() const
	{
		return m_name;
	}

	void QTModule::setDescription(const QString& description)
	{
		m_description = description;
	}
	const QString& QTModule::getDescription() const
	{
		return m_description;
	}
}