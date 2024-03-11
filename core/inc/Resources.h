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
		struct GitResources
		{
			QString repo;

			QString templateBranch;
			QString dependenciesBranch;
			QString qtModulesBranch;

			void load(const QJsonObject& obj);
			QJsonObject save() const;
		};
		static void loadSettings();
		static void saveSettings();
		static void loadQTModules();
		static void loadDependencies();

		static const QVector<QTModule> &getQTModules();
		static const QVector<Dependency> &getDependencies();
		static bool isOriginalDependency(const QString& name);

		static void setTemplateSourcePath(const QString& path);
		static const QString &getTemplateSourcePath();
		static QString getCurrentTemplateAbsSourcePath();

		static void setDependenciesSourcePath(const QString& path);
		static const QString &getDependenciesSourcePath();
		static QString getDependenciesAbsSourcePath();

		static void setQtModulesSourcePath(const QString& path);
		static const QString &getQtModulesSourcePath();	

		static void setStyleSheetSourcePath(const QString& path);
		static const QString &getStyleSheetSourcePath();
		static QString getStyleSheet();

		static void setTmpPath(const QString& path);
		static const QString &getTmpPath();

		static void setTemplateGitRepo(const GitResources& repo);
		static const GitResources& getTemplateGitRepo();

		static void setLoadedProjectPath(const QString& path);
		static const QString &getLoadedProjectPath();


	private:
		void loadSettings_intern();
		void saveSettings_intern();
		void loadQTModules_intern();
		void loadDependencies_intern();
		

		QJsonObject loadJsonFile(const QString& path);


		QString m_templateSourcePath;
		QString m_dependenciesSourcePath;
		QString m_qtModulesSourcePath;
		QString m_styleSheetSourcePath;
		QString m_tmpPath;
		
		GitResources m_gitRepo;

		QVector<QTModule> m_qtModules;
		QVector<Dependency> m_dependencies;

		QString m_loadedProjectPath;

		QString m_settingsFilePath;

	};
}