#include "ProjectSettings.h"


namespace CLC
{
	const ProjectSettings::Placeholder ProjectSettings::s_defaultPlaceholder = {
		"LibraryNamespace",
		"LIBRARY_NAME_EXPORT",
		"LibraryName",
		"LIBRARY_NAME_LIB",
		"LIBRARY_NAME_SHORT",
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

	ProjectSettings::LibrarySettings::LibrarySettings()
	{
		author = "Author";
		email = "";
		website = "";
		license = "MIT";
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