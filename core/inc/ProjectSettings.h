#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QVector>
#include "QTModule.h"
#include "Dependency.h"
#include "Utilities.h"

namespace CLC 
{
	class ProjectSettings
	{
	public:
		struct LibrarySettings
		{
			struct Version
			{
				int major = 0;
				int minor = 0;
				int patch = 0;
				Version();
			};
			Version version;
			QString author;
			QString email;
			QString website;
			QString license;

			QString namespaceName;
			QString apiName;
			LibrarySettings();

			void autoSetNamespaceName(const QString &libraryName);
			void autoSetApiName(const QString& libraryName);
		};
		struct CMAKE_settings
		{
			QString libraryName;
			QString lib_define;
			QString lib_profile_define;
			QString lib_short_define;

			bool qt_enable;
			bool qt_deploy;

			QString qt_installBase; // "C:/Qt"
			//int qt_major_version; // Qt5, Qt6
			//QString qt_version; // "5.15.2"
			QString qt_compiler; // "msvc2019_64"
			unsigned int qt_versionNr[3];
			bool qt_useNewestVersion;
			bool qt_autoFindCompiler;

			QVector<QTModule> qModules;
			QStringList customDefines;
			QStringList customGlobalDefines;

			QString debugPostFix;
			QString staticPostFix;
			QString profilingPostFix;

			unsigned int cxxStandard;
			bool cxxStandardRequired;

			bool compile_examples;
			bool compile_unittests;

			QVector<Dependency> dependencies;
			CMAKE_settings();
			void autosetLibDefine();
			void autosetLibProfileDefine();
			void autosetLibShortDefine();

			void setQtInstallBase(const QString& path);
			void setQtVersion(const QString& versionStr);
			QString getQtVersionStr() const;
			void setQtCompiler(const QString& compilerStr);
		};

		// Placeholder for strings that are used in the CMakeLists.txt and code files
		struct Placeholder
		{
			QString Library_Namespace;
			QString LIBRARY__NAME_API;
			QString Library_Name;
			QString LIBRARY__NAME_LIB;
			QString LIBRARY__NAME_SHORT;
		};

		struct CMakeFileUserSections
		{
			QString file;
			QVector<Utilities::UserSection> sections;
		};
		struct CodeUserSections
		{
			QString file;
			QVector<Utilities::UserSection> sections;
		};

		ProjectSettings();
		//ProjectSettings(const ProjectSettings &other);
		~ProjectSettings();

		ProjectSettings& operator=(const ProjectSettings& other);

		void setLibrarySettings(const LibrarySettings& settings);
		void setCMAKE_settings(const CMAKE_settings& settings);
		void setDefaultPlaceholder(const Placeholder& placeholder);
		void setLoadedPlaceholder(const Placeholder& placeholder);
		void setCmakeUserSections(const QVector<CMakeFileUserSections>& sections);
		void setCodeUserSections(const QVector<CodeUserSections>& sections);
		const LibrarySettings& getLibrarySettings() const;
		const CMAKE_settings& getCMAKE_settings() const;
		const Placeholder& getDefaultPlaceholder() const;
		const Placeholder& getLoadedPlaceholder() const;
		const QVector<CMakeFileUserSections>& getCmakeUserSections() const;
		const QVector<CodeUserSections>& getCodeUserSections() const;
		void autosetLibDefine();
		void autosetLibProfileDefine();
		void autosetLibShortDefine();
		void autoSetNamespaceName();
		void autoSetApiName();

		ProjectSettings getValidated() const;

		static const Placeholder s_defaultPlaceholder;
	private:


		LibrarySettings m_librarySettings;
		CMAKE_settings m_CMAKE_settings;
		Placeholder m_loadedPlaceholder;
		Placeholder m_defaultPlaceholder;

		QVector<CMakeFileUserSections> m_cmakeFileUserSections;
		QVector<CodeUserSections> m_codeUserSections;
	};
} // namespace CLC
