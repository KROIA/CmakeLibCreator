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
		m_loadedPlaceholder = s_defaultPlaceholder;
		m_defaultPlaceholder = s_defaultPlaceholder;
	}
	/*ProjectSettings::ProjectSettings(const ProjectSettings& other)
		: m_librarySettings(other.m_librarySettings)
		, m_CMAKE_settings(other.m_CMAKE_settings)
		, m_loadedPlaceholder(other.m_loadedPlaceholder)
		, m_defaultPlaceholder(other.m_loadedPlaceholder)
		, m_cmakeFileUserSections(other.m_cmakeFileUserSections)
		, m_codeUserSections(other.m_codeUserSections)
	{

	}
	*/
	ProjectSettings::~ProjectSettings()
	{
	}
	ProjectSettings& ProjectSettings::operator=(const ProjectSettings& other)
	{
		m_librarySettings = other.m_librarySettings;
		m_CMAKE_settings = other.m_CMAKE_settings;
		m_defaultPlaceholder = other.m_defaultPlaceholder;
		m_loadedPlaceholder = other.m_loadedPlaceholder;
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
	void ProjectSettings::setDefaultPlaceholder(const Placeholder& placeholder)
	{
		m_defaultPlaceholder = placeholder;
	}
	void ProjectSettings::setLoadedPlaceholder(const Placeholder& placeholder)
	{
		m_loadedPlaceholder = placeholder;
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
	const ProjectSettings::Placeholder& ProjectSettings::getDefaultPlaceholder() const
	{
		return m_defaultPlaceholder;
	}
	const ProjectSettings::Placeholder& ProjectSettings::getLoadedPlaceholder() const
	{
		return m_loadedPlaceholder;
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

	ProjectSettings ProjectSettings::getValidated() const
	{
		ProjectSettings v(*this);
		CMAKE_settings& cmakeSettings = v.m_CMAKE_settings;

		
		cmakeSettings.setQtCompiler(cmakeSettings.qt_compiler);
		cmakeSettings.setQtVersion(cmakeSettings.getQtVersionStr());



		return v;
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
	void ProjectSettings::CMAKE_settings::setQtInstallBase(const QString& path)
	{
		if (path.isEmpty())
			qt_installBase = "C:/";
		else
			qt_installBase = path;
	}
	void ProjectSettings::CMAKE_settings::setQtVersion(const QString& versionStr)
	{
		QStringList parts = versionStr.split('.');

		qt_versionNr[1] = 0;
		qt_versionNr[2] = 0;
		qt_useNewestVersion = false;
		if (parts.size() == 3) {
			qt_versionNr[0] = parts[0].toInt();
			qt_versionNr[1] = parts[1].toInt();
			qt_versionNr[2] = parts[2].toInt();
		}
		else {
			qt_useNewestVersion = true;
		}
		if (qt_versionNr[0] == 0)
			qt_versionNr[0] = 5;
	}
	QString ProjectSettings::CMAKE_settings::getQtVersionStr() const
	{
		if (qt_useNewestVersion)
		{
			return "autoFind";
		}
		return QString::number(qt_versionNr[0]) + "." + QString::number(qt_versionNr[1]) + "." + QString::number(qt_versionNr[2]);
	}
	void ProjectSettings::CMAKE_settings::setQtCompiler(const QString& compilerStr)
	{
		if (compilerStr.isEmpty() || compilerStr.toLower() == "autofind")
		{
			qt_autoFindCompiler = true;
			qt_compiler = "autoFind";
		}
		else
		{
			qt_autoFindCompiler = false;
			qt_compiler = compilerStr;
		}
	}

} // namespace CLC
