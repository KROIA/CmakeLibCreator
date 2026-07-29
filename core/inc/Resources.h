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
		struct ProjectEntry
		{
			QString path;
			bool groupEnabled = true;
		};
		struct LoadSaveProjects
		{
			QVector<ProjectEntry> projects;

			QStringList paths() const;          // convenience: all entry paths in order
			void load(const QJsonValue& val);   // accepts old (string array) and new (object array) formats
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
		static void setProjectGroupEnabled(const QString& path, bool enabled); // updates entry + saveSettings()

		static void setDefaultLibraryPath(const QString& path);
		static const QString& getDefaultLibraryPath();

		static void setMaxBuildThreads(int n);   // clamped to >= 1
		static int  getMaxBuildThreads();

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
		QString m_defaultLibraryPath;
		int m_maxBuildThreads = 4;

		QString m_settingsFilePath;

	};
}
