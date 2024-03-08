#include "Dependency.h"

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
}