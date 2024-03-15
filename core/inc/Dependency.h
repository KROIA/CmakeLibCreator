#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>

namespace CLC
{
	class Dependency
	{
	public:
		Dependency();
		Dependency(const QString& name, const QString& description);
		Dependency(const Dependency& other);
		~Dependency();

		Dependency& operator=(const Dependency& other);
		bool operator==(const Dependency& other) const;
		bool operator!=(const Dependency& other) const;

		void setName(const QString& name);
		const QString& getName() const;

		void setDescription(const QString& description);
		const QString& getDescription() const;

		bool loadFromCmakeFile(const QString& filePath);
	private:
		QString m_name;
		QString m_description;
	};
}
