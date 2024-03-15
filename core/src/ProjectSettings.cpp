#include "ProjectSettings.h"


namespace CLC
{
	// Needs to be splitted into 2 chuncks because otherwise this application would replace the strings in the file if 
	// This porject gets updatet with this tool.
	const ProjectSettings::Placeholder ProjectSettings::s_defaultPlaceholder = {
		QString("Library")+"Namespace",
		QString("LIBRARY")+"_NAME_EXPORT",
		QString("Library")+"Name",
		QString("LIBRARY")+"_NAME_LIB",
		QString("LIBRARY")+"_NAME_SHORT",
	};

	ProjectSettings::ProjectSettings()
	{
		m_librarySettings = LibrarySettings();
		m_CMAKE_settings = CMAKE_settings();
		m_placeholder = s_defaultPlaceholder;
	}

	ProjectSettings::~ProjectSettings()
	{
	}
	ProjectSettings& ProjectSettings::operator=(const ProjectSettings& other)
	{
		m_librarySettings = other.m_librarySettings;
		m_CMAKE_settings = other.m_CMAKE_settings;
		m_placeholder = other.m_placeholder;
		m_cmakeFileUserSections = other.m_cmakeFileUserSections;
		m_codeUserSections = other.m_codeUserSections;
		return *this;
	}
	void ProjectSettings::setLibrarySettings(const LibrarySettings& settings)
	{
		m_librarySettings = settings;
	}
	void ProjectSettings::setCMAKE_settings(const CMAKE_settings& settings)
	{
		m_CMAKE_settings = settings;
	}
	void ProjectSettings::setPlaceholder(const Placeholder& placeholder)
	{
		m_placeholder = placeholder;
	}
	void ProjectSettings::setCmakeUserSections(const QVector<CMakeFileUserSections>& sections)
	{
		m_cmakeFileUserSections = sections;
	}
	void ProjectSettings::setCodeUserSections(const QVector<CodeUserSections>& sections)
	{
		m_codeUserSections = sections;
	}
	const ProjectSettings::LibrarySettings& ProjectSettings::getLibrarySettings() const
	{
		return m_librarySettings;
	}
	const ProjectSettings::CMAKE_settings& ProjectSettings::getCMAKE_settings() const
	{
		return m_CMAKE_settings;
	}
	const ProjectSettings::Placeholder& ProjectSettings::getPlaceholder() const
	{
		return m_placeholder;
	}
	const QVector<ProjectSettings::CMakeFileUserSections>& ProjectSettings::getCmakeUserSections() const
	{
		return m_cmakeFileUserSections;
	}
	const QVector<ProjectSettings::CodeUserSections>& ProjectSettings::getCodeUserSections() const
	{
		return m_codeUserSections;
	}
	void ProjectSettings::autosetLibDefine()
	{
		m_CMAKE_settings.autosetLibDefine();
	}
	void ProjectSettings::autosetLibProfileDefine()
	{
		m_CMAKE_settings.autosetLibProfileDefine();
	}
	void ProjectSettings::autosetLibShortDefine()
	{
		m_CMAKE_settings.autosetLibShortDefine();
	}
	void ProjectSettings::autoSetNamespaceName()
	{
		m_librarySettings.autoSetNamespaceName(m_CMAKE_settings.libraryName);
	}
	void ProjectSettings::autoSetExportName()
	{
		m_librarySettings.autoSetExportName(m_CMAKE_settings.libraryName);
	}

	ProjectSettings::LibrarySettings::LibrarySettings()
	{
		author = "Author";
		email = "";
		website = "";
		license = "MIT";
	}
	void ProjectSettings::LibrarySettings::autoSetNamespaceName(const QString& libraryName)
	{
		namespaceName = libraryName;
	}
	void ProjectSettings::LibrarySettings::autoSetExportName(const QString& libraryName)
	{
		QString name;
		name.reserve(libraryName.size()*2);
		// convert each letter to upper case and if it was already uppercase, add a '_'
		for (int i = 0; i < libraryName.size(); i++)
		{
			if (libraryName[i] >= 'A' && libraryName[i] <= "Z" && i > 0)
			{
				if((libraryName[i - 1] >= 'a' && libraryName[i - 1] <= "z") ||
				   (libraryName[i - 1] >= '0' && libraryName[i - 1] <= "9"))
					name += '_';
			}
			name += libraryName[i].toUpper();
		}
		name += "_EXPORT";
		exportName = name;
	}
	ProjectSettings::LibrarySettings::Version::Version()
	{
		major = 1;
		minor = 0;
		patch = 0;
	}
	ProjectSettings::CMAKE_settings::CMAKE_settings()
	{
		libraryName = "MyLibrary";
		autosetLibDefine();
		autosetLibProfileDefine();
		autosetLibShortDefine();
		qt_enable = true;
		qt_deploy = true;
		qModules = QVector<QTModule>();

		debugPostFix = "-d";
		staticPostFix = "-s";
		profilingPostFix = "-p";
		cxxStandard = 17;
		cxxStandardRequired = true;
		compile_examples = true;
		compile_unittests = true;
		dependencies = QVector<Dependency>();
	}
	void ProjectSettings::CMAKE_settings::autosetLibDefine()
	{
		lib_define = libraryName.toUpper() + "_LIB";
	}
	void ProjectSettings::CMAKE_settings::autosetLibProfileDefine()
	{
		// Pick the first letter of each word in the library name. The first letter is always uppercase.
		QString shortName;
		for (int i = 0; i < libraryName.size(); i++)
		{
			if (libraryName[i] >= 'A' && libraryName[i] <= "Z")
			{
				shortName += libraryName[i];
			}
		}
		if (shortName.isEmpty())
		{
			shortName = libraryName[0].toUpper();
		}
		lib_profile_define = shortName + "_PROFILING";
	}
	void ProjectSettings::CMAKE_settings::autosetLibShortDefine()
	{
		// Pick the first letter of each word in the library name. The first letter is always uppercase.
		QString shortName;
		for (int i = 0; i < libraryName.size(); i++)
		{
			if (libraryName[i] >= 'A' && libraryName[i] <= "Z")
			{
				shortName += libraryName[i];
			}
		}
		if (shortName.isEmpty())
		{
			shortName = libraryName[0].toUpper();
		}
		lib_short_define = shortName;
	}

} // namespace CLC
