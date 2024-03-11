#include "Resources.h"
#include <QDir>
#include <QJsonDocument>
#include <QDebug>

namespace CLC
{
	Resources::Resources()
	{
		m_settingsFilePath = "settings.json";


		m_templateSourcePath = "data/template";
		m_dependenciesSourcePath = "data/depencencies";
		m_qtModulesSourcePath = "data/qtModules";
		m_styleSheetSourcePath = "data/stylesheet";
		m_tmpPath = "data/temp";

		m_gitRepo.repo = "https://github.com/KROIA/QT_cmake_library_template.git";
		m_gitRepo.templateBranch = "main";
		m_gitRepo.dependenciesBranch = "depencencies";
		m_gitRepo.qtModulesBranch = "qtModules";


		QDir dir;
		if(!dir.exists(m_templateSourcePath))
			dir.mkpath(m_templateSourcePath);
		if(!dir.exists(m_dependenciesSourcePath))
			dir.mkpath(m_dependenciesSourcePath);
		if (!dir.exists(m_qtModulesSourcePath))
			dir.mkpath(m_qtModulesSourcePath);
		if (!dir.exists(m_styleSheetSourcePath))
			dir.mkpath(m_styleSheetSourcePath);

		QFile file(m_settingsFilePath);
		if (!file.exists())
		{
			saveSettings_intern();
		}
		else
			loadSettings_intern();
		//loadQTModules();
		//loadDependencies();
	}
	Resources& Resources::instance()
	{
		static Resources instance;
		return instance;
	}

	void Resources::loadSettings()
	{
		instance().loadSettings_intern();
	}
	void Resources::saveSettings()
	{
		instance().saveSettings_intern();
	}
	void Resources::loadQTModules()
	{
		instance().loadQTModules_intern();
	}
	void Resources::loadDependencies()
	{
		instance().loadDependencies_intern();
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
	QString Resources::getCurrentTemplateAbsSourcePath()
	{
		const Resources& res = instance();
		return QDir::currentPath() + "/" + res.m_templateSourcePath + "/" + res.m_gitRepo.templateBranch;
	}

	void Resources::setDependenciesSourcePath(const QString& path)
	{
		instance().m_dependenciesSourcePath = path;
		instance().loadDependencies_intern();
	}
	const QString &Resources::getDependenciesSourcePath()
	{
		return instance().m_dependenciesSourcePath;
	}
	void Resources::setQtModulesSourcePath(const QString& path)
	{
		instance().m_qtModulesSourcePath = path;	
		instance().loadQTModules_intern();
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

	void Resources::setTemplateGitRepo(const GitResources& repo)
	{
		instance().m_gitRepo = repo;
	}
	const Resources::GitResources& Resources::getTemplateGitRepo()
	{
		return instance().m_gitRepo;
	}
	void Resources::setLoadedProjectPath(const QString& path)
	{
		instance().m_loadedProjectPath = path;
	}
	const QString& Resources::getLoadedProjectPath()
	{
		return instance().m_loadedProjectPath;
	}
	void Resources::loadSettings_intern()
	{
		QJsonObject settings;
		QFile file(m_settingsFilePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			qDebug() << "Failed to open file for reading: " << file.fileName();
			return;
		}
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		file.close();
		settings = doc.object();
		m_templateSourcePath = settings["templateSourcePath"].toString();
		m_dependenciesSourcePath = settings["dependenciesSourcePath"].toString();
		m_qtModulesSourcePath = settings["qtModulesSourcePath"].toString();
		m_styleSheetSourcePath = settings["styleSheetSourcePath"].toString();
		m_tmpPath = settings["tmpPath"].toString();

		m_gitRepo.load(settings["git"].toObject());
		loadQTModules_intern();
		loadDependencies_intern();
	}
	void Resources::saveSettings_intern()
	{
		QJsonObject settings;
		settings["templateSourcePath"] = m_templateSourcePath;
		settings["dependenciesSourcePath"] = m_dependenciesSourcePath;
		settings["qtModulesSourcePath"] = m_qtModulesSourcePath;
		settings["styleSheetSourcePath"] = m_styleSheetSourcePath;
		settings["tmpPath"] = m_tmpPath;
		settings["git"] = m_gitRepo.save();

		QJsonDocument doc(settings);
		QFile file(m_settingsFilePath);
		if (!file.open(QIODevice::WriteOnly))
		{
			qDebug() << "Failed to open file for writing: " << file.fileName();
			return;
		}
		file.write(doc.toJson());
		file.close();
	}

	void Resources::GitResources::load(const QJsonObject& obj)
	{
		repo = obj["repo"].toString();
		templateBranch = obj["templateBranch"].toString();
		dependenciesBranch = obj["dependenciesBranch"].toString();
		qtModulesBranch = obj["qtModulesBranch"].toString();
	}
	QJsonObject Resources::GitResources::save() const
	{
		QJsonObject obj;
		obj["repo"] = repo;
		obj["templateBranch"] = templateBranch;
		obj["dependenciesBranch"] = dependenciesBranch;
		obj["qtModulesBranch"] = qtModulesBranch;
		return obj;
	}
	void Resources::loadQTModules_intern()
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
	void Resources::loadDependencies_intern()
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