#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QJsonObject>

namespace CLC
{
	class QTModule
	{
	public:
		QTModule();
		QTModule(const QString name, const QString description);
		QTModule(const QTModule& other);
		~QTModule();

		QTModule& operator=(const QTModule& other);
		bool operator==(const QTModule& other) const;
		bool operator!=(const QTModule& other) const;

		void setName(const QString& name);
		const QString &getName() const;

		void setDescription(const QString& description);
		const QString &getDescription() const;

		bool loadFromJson(const QJsonObject& json);
		QJsonObject toJson() const;
	private:
		QString m_name;
		QString m_description;
	};
}