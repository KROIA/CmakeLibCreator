#include "Resources.h"
#include <QDir>

namespace CLC
{
	Resources::Resources()
	{
		m_templateSourcePath = "data/template";
		m_librarySourcePath = "data/libraries";
		m_tmpPath = "data/temp";
		m_templateGitRepo = "https://github.com/KROIA/QT_cmake_library_template.git";

		QDir dir;
		if(!dir.exists(m_templateSourcePath))
		{
			dir.mkpath(m_templateSourcePath);
		}
		if(!dir.exists(m_librarySourcePath))
		{
			dir.mkpath(m_librarySourcePath);
		}
	}
	Resources& Resources::instance()
	{
		static Resources instance;
		return instance;
	}
	QVector<QTModule> Resources::getQTModules()
	{
		QVector<QTModule> modules;
		modules.append(QTModule("Core", "QtCore"));
		modules.append(QTModule("Gui", "QtGui"));
		modules.append(QTModule("Widgets", "QtWidgets"));
		modules.append(QTModule("Network", "QtNetwork"));
		modules.append(QTModule("OpenGL", "QtOpenGL"));
		modules.append(QTModule("Multimedia", "QtMultimedia"));
		modules.append(QTModule("MultimediaWidgets", "QtMultimediaWidgets"));
		modules.append(QTModule("Sql", "QtSql"));
		modules.append(QTModule("Xml", "QtXml"));
		modules.append(QTModule("XmlPatterns", "QtXmlPatterns"));
		modules.append(QTModule("WebKit", "QtWebKit"));
		modules.append(QTModule("WebKitWidgets", "QtWebKitWidgets"));
		modules.append(QTModule("Script", "QtScript"));
		modules.append(QTModule("ScriptTools", "QtScriptTools"));
		modules.append(QTModule("Svg", "QtSvg"));
		modules.append(QTModule("Test", "QtTest"));
		modules.append(QTModule("UiTools", "QtUiTools"));
		modules.append(QTModule("Designer", "QtDesigner"));
		modules.append(QTModule("Help", "QtHelp"));
		modules.append(QTModule("AxContainer", "QtAxContainer"));
		modules.append(QTModule("AxServer", "QtAxServer"));
		modules.append(QTModule("AxBase", "QtAxBase"));
		modules.append(QTModule("AxContainer", "QtAxContainer"));
		modules.append(QTModule("AxServer", "QtAxServer"));
		modules.append(QTModule("AxBase", "QtAxBase"));
		return modules;
	}
	QVector<Dependency> Resources::getDependencies()
	{
		QVector<Dependency> dependencies;
		dependencies.append(Dependency("easy_profiler", "easy_profiler library"));
		dependencies.append(Dependency("RibbonWidget",  "RibbonWidget library"));

		return dependencies;
	}

	void Resources::setTemplateSourcePath(const QString& path)
	{
		instance().m_templateSourcePath = path;
	}
	const QString &Resources::getTemplateSourcePath()
	{
		return instance().m_templateSourcePath;
	}

	void Resources::setLibrarySourcePath(const QString& path)
	{
		instance().m_librarySourcePath = path;
	}
	const QString &Resources::getLibrarySourcePath()
	{
		return instance().m_librarySourcePath;
	}
	void Resources::setTmpPath(const QString& path)
	{
		instance().m_tmpPath = path;	
	}
	const QString& Resources::getTmpPath()
	{
		return instance().m_tmpPath;
	}

	void Resources::setTemplateGitRepo(const QString& repo)
	{
		instance().m_templateGitRepo = repo;
	}
	const QString& Resources::getTemplateGitRepo()
	{
		return instance().m_templateGitRepo;
	}
}