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

			void load(const QJsonValue& val);
			QJsonValue save() const;
		};
		struct LoadSaveProjects
		{
			QStringList projectPaths;

			void load(const QJsonValue& val);
			QJsonValue save() const;
		};
		static void loadSettings();
		static void saveSettings();
		static void loadQTModules();
		static void loadDependencies();

		static const QVector<QTModule> &getQTModules();
		static const QVector<Dependency> &getDependencies();
		static bool isOriginalDependency(const QString& name);

		static void setRelativeTemplateSourcePath(const QString& path);
		static const QString &getRelativeTemplateSourcePath();
		static QString getCurrentTemplateAbsSourcePath();

		static void setRelativeDependenciesSourcePath(const QString& path);
		static const QString &getRelativeDependenciesSourcePath();
		static QString getDependenciesAbsSourcePath();

		static void setRelativeQtModulesSourcePath(const QString& path);
		static const QString &getRelativeQtModulesSourcePath();

		static void setRelativeStyleSheetSourcePath(const QString& path);
		static const QString &getRelativeStyleSheetSourcePath();
		static QString getStyleSheet();

		static void setRelativeTmpPath(const QString& path);
		static const QString &getRelativeTmpPath();

		static void setTemplateGitRepo(const GitResources& repo);
		static const GitResources& getTemplateGitRepo();

		static void setLoadSaveProjects(const LoadSaveProjects& paths);
		static const LoadSaveProjects& getLoadSaveProjects();

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
		LoadSaveProjects m_loadSaveProjects;

		QVector<QTModule> m_qtModules;
		QVector<Dependency> m_dependencies;

		QString m_loadedProjectPath;

		QString m_settingsFilePath;

	};
}
