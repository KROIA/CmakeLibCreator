#pragma once

#include "CmakeLibraryCreator_base.h"
#include "QTModule.h"
#include "Dependency.h"
#include <QVector>
#include <QJsonObject>


namespace CLC
{
	class Resources
	{
		Resources();
		Resources(const Resources&) = delete;

		static Resources& instance();
	public:
		static QVector<QTModule> getQTModules();
		static QVector<Dependency> getDependencies();

		static void setTemplateSourcePath(const QString& path);
		static const QString &getTemplateSourcePath();

		static void setLibrarySourcePath(const QString& path);
		static const QString &getLibrarySourcePath();

		static void setTmpPath(const QString& path);
		static const QString &getTmpPath();

		static void setTemplateGitRepo(const QString& repo);
		static const QString &getTemplateGitRepo();
	private:
		void loadQTModules();
		void loadDependencies();
		QJsonObject loadJsonFile(const QString& path);


		QString m_templateSourcePath;
		QString m_dependenciesSourcePath;
		QString m_qtModulesSourcePath;
		QString m_tmpPath;
		
		QString m_templateGitRepo;

		QVector<QTModule> m_qtModules;
		QVector<Dependency> m_dependencies;

	};
}