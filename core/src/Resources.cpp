#include "Resources.h"
#include <QDir>
#include <QJsonDocument>
#include <QDebug>

namespace CLC
{
	Resources::Resources()
	{
		m_templateSourcePath = "data/template";
		m_dependenciesSourcePath = "data/depencencies";
		m_qtModulesSourcePath = "data/qtModules";
		m_styleSheetSourcePath = "data/stylesheet";
		m_tmpPath = "data/temp";
		m_templateGitRepo = "https://github.com/KROIA/QT_cmake_library_template.git";

		QDir dir;
		if(!dir.exists(m_templateSourcePath))
			dir.mkpath(m_templateSourcePath);
		if(!dir.exists(m_dependenciesSourcePath))
			dir.mkpath(m_dependenciesSourcePath);
		if (!dir.exists(m_qtModulesSourcePath))
			dir.mkpath(m_qtModulesSourcePath);
		if (!dir.exists(m_styleSheetSourcePath))
			dir.mkpath(m_styleSheetSourcePath);
		loadQTModules();
		loadDependencies();
	}
	Resources& Resources::instance()
	{
		static Resources instance;
		return instance;
	}
	QVector<QTModule> Resources::getQTModules()
	{
		return instance().m_qtModules;
	}
	QVector<Dependency> Resources::getDependencies()
	{
		return instance().m_dependencies;
	}

	void Resources::setTemplateSourcePath(const QString& path)
	{
		instance().m_templateSourcePath = path;
	}
	const QString &Resources::getTemplateSourcePath()
	{
		return instance().m_templateSourcePath;
	}

	void Resources::setDependenciesSourcePath(const QString& path)
	{
		instance().m_dependenciesSourcePath = path;
		instance().loadDependencies();
	}
	const QString &Resources::getDependenciesSourcePath()
	{
		return instance().m_dependenciesSourcePath;
	}
	void Resources::setQtModulesSourcePath(const QString& path)
	{
		instance().m_qtModulesSourcePath = path;	
		instance().loadQTModules();
	}
	const QString& Resources::getQtModulesSourcePath()
	{
		return instance().m_qtModulesSourcePath;
	}

	void Resources::setStyleSheetSourcePath(const QString& path)
	{
		instance().m_styleSheetSourcePath = path;
	}
	const QString& Resources::getStyleSheetSourcePath()
	{
		return instance().m_styleSheetSourcePath;
	}
	QString Resources::getStyleSheet()
	{
		QString path = getStyleSheetSourcePath();
		// search for the first file in the directory
		QDir dir(path);
		QStringList files = dir.entryList(QStringList() << "*.*", QDir::Files);
		if (files.size() == 0)
		{
			//qDebug() << "No qss file found in: " << path;
			return QString();
		}
		QFile file(path + "/" + files[0]);
		file.open(QFile::ReadOnly);
		QString styleSheet = QLatin1String(file.readAll());
		return styleSheet;
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
	void Resources::loadQTModules()
	{
		/*QVector<QTModule> modules;
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

		for (const QTModule& module : modules)
		{
			QJsonObject json = module.toJson();
			QFile file(m_qtModulesSourcePath + "/" + module.getName() + ".json");
			if (!file.open(QIODevice::WriteOnly))
			{
				qDebug() << "Failed to open file for writing: " << file.fileName();
				continue;
			}
			QJsonDocument doc(json);
			file.write(doc.toJson());
			file.close();
		}*/


		m_qtModules.clear();
		// Load from json file
		QStringList files = QDir(m_qtModulesSourcePath).entryList(QStringList() << "*.json", QDir::Files);
		//Sort files alphabeticly
		std::sort(files.begin(), files.end());
		for (const QString& file : files)
		{
			QJsonObject json = loadJsonFile(m_qtModulesSourcePath + "/" + file);
			if (json.isEmpty())
			{
				qDebug() << "Failed to load json file: " << file;
				continue;
			}
			QTModule module;
			if(module.loadFromJson(json))
				m_qtModules.append(module);
			else
				qDebug() << "Failed to load module from json file: " << file << " data: "<< json;
		}
	}
	void Resources::loadDependencies()
	{
		QStringList files = QDir(m_dependenciesSourcePath).entryList(QStringList() << "*.cmake", QDir::Files);
		//Sort files alphabeticly
		std::sort(files.begin(), files.end());
		m_dependencies.clear();
		for (const QString& file : files)
		{
			Dependency dep;
			if (dep.loadFromCmakeFile(m_dependenciesSourcePath + "/" + file))
			{
				m_dependencies.append(dep);
			}
			else
				qDebug() << "Failed to load dependency from cmake file: " << file;
		}
	}
	QJsonObject Resources::loadJsonFile(const QString& path)
	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
		{
			return QJsonObject();
		}
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		file.close();
		return doc.object();
	}
}