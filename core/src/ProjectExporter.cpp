#include "ProjectExporter.h"
#include <QDir>
#include "Utilities.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace CLC
{
	

	ProjectExporter::ProjectExporter()
	{

	}
	ProjectExporter& ProjectExporter::instance()
	{
		static ProjectExporter instance;
		return instance;
	}

	bool ProjectExporter::exportProject(
		const ProjectSettings& settings, 
		const QString& templateSourceDir,
		const QString& projectDirPath,
		const ExportSettings& expSettings)
	{
		return instance().exportProject_intern(settings, templateSourceDir, projectDirPath, expSettings);
	}
	bool ProjectExporter::readProjectData(ProjectSettings& settings, const QString& projectDirPath)
	{
		return instance().readProjectData_intern(settings, projectDirPath);
	}

	bool ProjectExporter::exportProject_intern(
		const ProjectSettings& settings, 
		const QString& templateSourceDir,
		const QString& projectDirPath,
		const ExportSettings& expSettings)
	{
		bool success = true;
		//const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		//const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();

		if (expSettings.copyTemplateFiles)
		{
			// Create the project directory
			QDir dir;
			QDir projectDir(projectDirPath);
			if (dir.exists(projectDirPath))
			{
				if (!QMessageBox::question(0, "Folder already exists", "The folder already exists. Do you want to overwrite it?"))
					return false;
				success &= Utilities::deleteFolderRecursively(projectDirPath);
			}

			if (!dir.mkpath(projectDirPath))
			{
				QMessageBox::critical(0, "Error", "Failed to create project directory:\n" + projectDirPath);
				return false;
			}

			if (success) success &= copyTemplateSourceFiles(settings, templateSourceDir, projectDirPath);
		}
		if (expSettings.replaceTemplateFiles)
		{
			if (success) success &= copyTemplateLibraryFiles(settings, templateSourceDir, projectDirPath);
			if (success) success &= replaceTemplateFileNames(settings, projectDirPath);
		}
		if (expSettings.replaceTemplateVariables)
		{
			if (success) success &= replaceTemplateVariables(settings, projectDirPath);
		}
		if (expSettings.replaceTemplateCodePlaceholders)
		{
			if (success) success &= replaceTemplateCodePlaceholders(settings, projectDirPath);
		}	
		if (success) success &= replaceTemplateUserSectionsIn_cmakeLists(settings, projectDirPath);
		
		return success;
	}
	bool ProjectExporter::readProjectData_intern(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		if(success) success &= readCmakeLists(settings, projectDirPath);
		if(!success) return false;
		success &= readLibraryInfo(settings, projectDirPath);
		success &= readDependencies(settings, projectDirPath);



		ProjectSettings::Placeholder placeholder;
		const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();
		placeholder.LibraryNamespace = librarySettings.namespaceName;
		placeholder.LIBRARY_NAME_EXPORT = librarySettings.exportName;
		placeholder.LibraryName = cmakeSettings.libraryName;
		placeholder.LIBRARY_NAME_LIB = cmakeSettings.lib_define;
		placeholder.LIBRARY_NAME_SHORT = cmakeSettings.lib_short_define;
		settings.setPlaceholder(placeholder);
		return success;
	}

	bool ProjectExporter::copyTemplateLibraryFiles(
		const ProjectSettings& settings, 
		const QString& templateSourceDir,
		const QString& projectDirPath)
	{
		
		bool success = true;
		QVector<QString> sourceDirs{
			"cmake",
		};

		for (const auto& sourceDir : sourceDirs)
		{
			QString source = templateSourceDir + "/" + sourceDir;
			QString target = projectDirPath + "/" + sourceDir;
			success &= Utilities::copyAndReplaceFolderContents(source, target);
		}

		QString depsPath = templateSourceDir + "/dependencies";
		if (!Utilities::createFolder(projectDirPath + "/dependencies"))
		{
			QMessageBox::critical(0, "Error", "Failed to create folder:\n" + depsPath);
			return false;
		}
		if (!Utilities::copyFile(depsPath + "/.gitignore", projectDirPath + "/dependencies/.gitignore", true))
		{
			QMessageBox::critical(0, "Error", "Failed to copy file:\n" + depsPath + "/.gitignore");
			return false;
		}

		// Remove all existing dependencies
		QVector<QString> existingDeps = Utilities::getFilesInFolder(projectDirPath + "/dependencies/", ".cmake");
		for (const auto& dep : existingDeps)
		{
			success &= Utilities::deleteFile(dep);
		}
		// Remove cache
		QVector<QString> cachePaths = Utilities::getFoldersInFolder(projectDirPath + "/dependencies/");
		for (const auto& cache : cachePaths)
		{
			success &= Utilities::deleteFolderRecursively(cache);
		}
		
		QVector<Dependency> dependencies = settings.getCMAKE_settings().dependencies;
		for (const auto& dep : dependencies)
		{
			QString depFileName = dep.getName() + ".cmake";
			QString source = templateSourceDir + "/dependencies/" + depFileName;
			QString target = projectDirPath + "/dependencies/" + depFileName;
			if (!Utilities::copyFile(source, target, true))
			{
				QMessageBox::critical(0, "Error", "Failed to copy file:\n" + source);
				//return false;
			}
		}

		QVector<QString> fileList{
			".gitignore",
			"CMakeSettings.json",
			"build.bat",
			"CMakeLists.txt",
		};
		for (const auto& file : fileList)
		{
			QString source = templateSourceDir + "/" + file;
			QString target = projectDirPath + "/" + file;
			if (!Utilities::copyFile(source, target, true))
			{
				QMessageBox::critical(0, "Error", "Failed to copy file:\n" + source);
				//return false;
			}
		}

		// override the core CMakeLists.txt
		if (!Utilities::copyFile(templateSourceDir+"/core/CMakeLists.txt", projectDirPath + "/core/CMakeLists.txt", true))
		{
			QMessageBox::critical(0, "Error", "Failed to copy file:\n" + templateSourceDir + "/core/CMakeLists.txt");
		}
		

		return success;
	}
	bool ProjectExporter::copyTemplateSourceFiles(const ProjectSettings& settings,
		const QString& templateSourceDir,
		const QString& projectDirPath)
	{
		CLC_UNUSED(settings);
		bool success = true;
		QVector<QString> sourceDirs{
			"core",
			"examples",
			"unittests",
		};

		for (const auto& sourceDir : sourceDirs)
		{
			QString source = templateSourceDir + "/" + sourceDir;
			QString target = projectDirPath + "/" + sourceDir;
			success &= Utilities::copyAndReplaceFolderContents(source, target);
		}

		return success;
	}

	bool ProjectExporter::replaceTemplateVariables(const ProjectSettings& settings,
												   const QString& projectDirPath)
	{
		bool success = true;
		if(success) success &= replaceTemplateVariablesIn_mainCmakeLists(settings, projectDirPath);
		if(success) success &= replaceTemplateVariablesIn_cmakeSettings(settings, projectDirPath);
		if(success) success &= replaceTemplateVariablesIn_libraryInfo(settings, projectDirPath);

		return success;
	}
	bool ProjectExporter::replaceTemplateFileNames(
		const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		QString targetFileNameContains = settings.getPlaceholder().LibraryName;
		QVector<QString> fileList1 = Utilities::getFilesInFolderRecursive(projectDirPath, ".h");
		QVector<QString> fileList2 = Utilities::getFilesInFolderRecursive(projectDirPath, ".cpp");
		QVector<QString> allFiles = fileList1 + fileList2;

		int counter = 0;
		for (const auto& file : allFiles)
		{
			QString fileName = QFileInfo(file).fileName();
			if (fileName.contains(targetFileNameContains))
			{
				QString newFileName = QFileInfo(file).fileName();
				newFileName.replace(settings.getPlaceholder().LibraryName, settings.getCMAKE_settings().libraryName);
				newFileName = QFileInfo(file).absolutePath() + "/" + newFileName;
				if (file != newFileName)
				{
					QFile::rename(file, newFileName);
					counter++;
				}
			}
		}
		qDebug() << "Renamed " << counter << " files";
		return true;
	}

	bool ProjectExporter::replaceTemplateVariablesIn_mainCmakeLists(
		const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		bool success = true;
		QString cmakeListsPath = projectDirPath + "/CMakeLists.txt";
		QVector<QString> fileContent = Utilities::getFileContents(cmakeListsPath);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n"+ cmakeListsPath);
			return false;
		}

		//const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();

		// Replace the library name
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "LIBRARY_NAME", cmakeSettings.libraryName);
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "LIB_DEFINE", cmakeSettings.lib_define);
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "LIB_PROFILE_DEFINE", cmakeSettings.lib_profile_define);

		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "QT_ENABLE", (cmakeSettings.qt_enable?"ON":"OFF"));
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "QT_DEPLOY", (cmakeSettings.qt_deploy?"ON":"OFF"));

		QVector<QString> qtModules;
		for (const auto& module : cmakeSettings.qModules)
		{
			qtModules.push_back(module.getName());
		}
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "QT_MODULES", qtModules);

		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "DEBUG_POSTFIX_STR", cmakeSettings.debugPostFix);
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "STATIC_POSTFIX_STR", cmakeSettings.staticPostFix);
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "PROFILING_POSTFIX_STR", cmakeSettings.profilingPostFix);

		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", QString::number(cmakeSettings.cxxStandard));
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", (cmakeSettings.cxxStandardRequired?"ON":"OFF"));

		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "COMPILE_EXAMPLES", (cmakeSettings.compile_examples?"ON":"OFF"));
		if(success) success &= Utilities::replaceCmakeVariable(fileContent, "COMPILE_UNITTESTS", (cmakeSettings.compile_unittests?"ON":"OFF"));

		if(success) success &= Utilities::saveFileContents(cmakeListsPath, fileContent);

		return success;
	}

	bool ProjectExporter::replaceTemplateVariablesIn_cmakeSettings(const ProjectSettings& settings,
																   const QString& projectDirPath)
	{
		QString cmakeListsPath = projectDirPath + "/CMakeSettings.json";
		// Read json file:
		QJsonDocument doc;
		QFile file(cmakeListsPath);
		if (!file.open(QIODevice::ReadOnly))
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n" + cmakeListsPath);
			return false;
		}
		doc = QJsonDocument::fromJson(file.readAll());
		file.close();
		QJsonObject obj = doc.object();
		QJsonArray configurations = obj["configurations"].toArray();

		// Search configuration with name "x64-Release-Profile"
		for (int i = 0; i < configurations.size(); i++)
		{
			QJsonObject config = configurations[i].toObject();
			if (config["name"].toString() == "x64-Release-Profile" ||
				config["name"].toString() == "x64-Debug-Profile")
			{
				config["cmakeCommandArgs"] = "-D" + settings.getCMAKE_settings().lib_profile_define + "=1";
				configurations[i] = config;
			}
		}
		obj["configurations"] = configurations;
		doc.setObject(obj);
		if (!file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(0, "Error", "Failed to write file:\n" + cmakeListsPath);
			return false;
		}
		file.write(doc.toJson());
		file.close();
		return true;
	}

	bool ProjectExporter::replaceTemplateVariablesIn_libraryInfo(const ProjectSettings& settings,
																 const QString& projectDirPath)
	{
		bool success = true;
		QString header = projectDirPath + "/core/inc/" + settings.getCMAKE_settings().libraryName + "_info.h";
		QVector<QString> fileContent = Utilities::getFileContents(header);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n" + header);
			return false;
		}

		const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings& cmakeSettings = settings.getCMAKE_settings();

		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "versionMajor", QString::number(librarySettings.version.major));
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "versionMinor", QString::number(librarySettings.version.minor));
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "versionPatch", QString::number(librarySettings.version.patch));

		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "name", "\"" + cmakeSettings.libraryName + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "author", "\"" + librarySettings.author + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "email", "\"" + librarySettings.email + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "website", "\"" + librarySettings.website + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "license", "\"" + librarySettings.license + "\"");



		if(success) success &= Utilities::saveFileContents(header, fileContent);

		return success;
	}

	bool ProjectExporter::replaceTemplateCodePlaceholders(const ProjectSettings& settings,
														  const QString& projectDirPath)
	{
		struct File
		{
			QString path;
			QVector<QString> data;
		};
		QVector<File> files;
		QVector<QString> fileList1 = Utilities::getFilesInFolderRecursive(projectDirPath, ".h");
		QVector<QString> fileList2 = Utilities::getFilesInFolderRecursive(projectDirPath, ".cpp");
		for (const auto& file : fileList1)
		{
			File f;
			f.path = file;
			f.data = Utilities::getFileContents(file);
			files.push_back(f);
		}
		for (const auto& file : fileList2)
		{
			File f;
			f.path = file;
			f.data = Utilities::getFileContents(file);
			files.push_back(f);
		}
		const ProjectSettings::LibrarySettings& librarySettigns = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings& cmakeSettings = settings.getCMAKE_settings();
		const ProjectSettings::Placeholder& placeholers = settings.getPlaceholder();
		
		QVector<QPair<QString, QString>> replacements{
			{placeholers.LibraryNamespace,librarySettigns.namespaceName},
			{placeholers.LIBRARY_NAME_EXPORT,librarySettigns.exportName},
			{placeholers.LIBRARY_NAME_SHORT,cmakeSettings.lib_short_define},
			{placeholers.LIBRARY_NAME_LIB,cmakeSettings.lib_define},
		};

		for (auto& file : files)
		{
			for (const auto& replacement : replacements)
			{
				if(replacement.first != replacement.second)
					Utilities::replaceAll(file.data, replacement.first, replacement.second);
			}
			if(placeholers.LibraryName != cmakeSettings.libraryName)
				Utilities::replaceAllIfLineContains(file.data, placeholers.LibraryName, cmakeSettings.libraryName, "#include");
			Utilities::saveFileContents(file.path, file.data);
		}
		return true;
	}
	bool ProjectExporter::replaceTemplateUserSectionsIn_cmakeLists(
		const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		CLC_UNUSED(projectDirPath);
		//QVector<QString> smakeListsFiles = Utilities::getFilesInFolderRecursive(projectDirPath, "CMakeLists.txt");
		const QVector<ProjectSettings::QMakeFileUserSections> &userSections = settings.getUserSections();
		bool success = true;
		for (const ProjectSettings::QMakeFileUserSections& sectionList : userSections)
		{
			// Check if the file exists
			if (!QFile::exists(sectionList.file))
			{
				QMessageBox::critical(0, "Error", "Failed to find file:\n" + sectionList.file+"\nto replace user sections");
				success &= false;
				continue;
			}
			success &= Utilities::replaceCmakeUserSections(sectionList.file, sectionList.sections);
		}
		return success;
	}


	/*bool ProjectExporter::saveConfiguration(const ProjectSettings& settings,
											const QString& projectDirPath)
	{
		bool success = true;
		QString cmakeListsPath = projectDirPath + "/CMakeLists.txt";
		QVector<QString> fileContent = Utilities::getFileContents(cmakeListsPath);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n" + cmakeListsPath);
			return false;
		}
	}*/

	bool ProjectExporter::readCmakeLists(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		QString cmakeListsPath = projectDirPath + "/CMakeLists.txt";
		if (QFile::exists(cmakeListsPath) == false)
		{
			QMessageBox::critical(0, "Error", "Failed to find file:\n" + cmakeListsPath+"\nIs this a library directory?");
			return false;
		}
		QVector<QString> fileContent = Utilities::getFileContents(cmakeListsPath);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n" + cmakeListsPath);
			return false;
		}

		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		success &= Utilities::readCmakeVariable(fileContent, "LIBRARY_NAME", cmakeSettings.libraryName);
		success &= Utilities::readCmakeVariable(fileContent, "LIB_DEFINE", cmakeSettings.lib_define);
		success &= Utilities::readCmakeVariable(fileContent, "LIB_PROFILE_DEFINE", cmakeSettings.lib_profile_define);

		success &= Utilities::readCmakeVariable(fileContent, "QT_ENABLE", cmakeSettings.qt_enable);
		success &= Utilities::readCmakeVariable(fileContent, "QT_DEPLOY", cmakeSettings.qt_deploy);

		QVector<QString> qtModules;
		success &= Utilities::readCmakeVariables(fileContent, "QT_MODULES", qtModules);
		cmakeSettings.qModules.clear();
		for (const auto& module : qtModules)
		{
			cmakeSettings.qModules.push_back(QTModule(module, ""));
		}

		success &= Utilities::readCmakeVariable(fileContent, "DEBUG_POSTFIX_STR", cmakeSettings.debugPostFix);
		success &= Utilities::readCmakeVariable(fileContent, "STATIC_POSTFIX_STR", cmakeSettings.staticPostFix);
		success &= Utilities::readCmakeVariable(fileContent, "PROFILING_POSTFIX_STR", cmakeSettings.profilingPostFix);


		success &= Utilities::readCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", cmakeSettings.cxxStandard);
		success &= Utilities::readCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", cmakeSettings.cxxStandardRequired);

		if (success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", QString::number(cmakeSettings.cxxStandard));
		if (success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", (cmakeSettings.cxxStandardRequired ? "ON" : "OFF"));

		success &= Utilities::readCmakeVariable(fileContent, "COMPILE_EXAMPLES", cmakeSettings.compile_examples);
		success &= Utilities::readCmakeVariable(fileContent, "COMPILE_UNITTESTS", cmakeSettings.compile_unittests);

		settings.setCMAKE_settings(cmakeSettings);

		// Read User sections
		QVector<QString> cmakeListsFiles = Utilities::getFilesInFolderRecursive(projectDirPath, "CMakeLists.txt");
		QVector<ProjectSettings::QMakeFileUserSections> userSections;
		for (const auto& file : cmakeListsFiles)
		{
			QVector<Utilities::CmakeUserSection> sections;
			Utilities::readCmakeUserSections(file, sections);
			if (sections.size() > 0)
			{
				ProjectSettings::QMakeFileUserSections userSection;
				userSection.file = file;
				userSection.sections = sections;
				userSections.push_back(userSection);
			}
		}
		settings.setUserSections(userSections);

		return success;
	}
	bool ProjectExporter::readLibraryInfo(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		ProjectSettings::LibrarySettings librarySettings = settings.getLibrarySettings();

		QString header = projectDirPath + "/core/inc/" + cmakeSettings.libraryName + "_info.h";
		QVector<QString> fileContent = Utilities::getFileContents(header);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n" + header);
			return false;
		}

		// Read the namespace from that file
		QString namespaceKey = "namespace";
		int namespaceLineIndex = Utilities::getLineIndex(fileContent, namespaceKey, true);
		if (namespaceLineIndex == -1)
		{
			QMessageBox::critical(0, "Error", "Could not find namespace in " + header);
			//return false;
		}
		QString namespaceName = fileContent[namespaceLineIndex].mid(fileContent[namespaceLineIndex].indexOf(namespaceKey)+ namespaceKey.size()+1).trimmed();
		librarySettings.namespaceName = namespaceName;

		// Read the export name from that file
		int exportLineIndex1 = Utilities::getLineIndex(fileContent, "EXPORT", false);
		int exportLineIndex2 = Utilities::getLineIndex(fileContent, { "class","LibraryInfo" }, true);
		if ((exportLineIndex1 == -1 || exportLineIndex2 == -1) || (exportLineIndex1 != exportLineIndex2))
		{
			QMessageBox::critical(0, "Error", "Could not find export name in " + header);
			//return false;
		}
		else
		{
			QString line = fileContent[exportLineIndex1];
			line = line.mid(line.indexOf("class") + 6);
			QString exportName = line.mid(0, line.indexOf("LibraryInfo")).trimmed();
			librarySettings.exportName = exportName;
		}

		success &= Utilities::readHeaderVariable(fileContent, "versionMajor", librarySettings.version.major);
		success &= Utilities::readHeaderVariable(fileContent, "versionMinor", librarySettings.version.minor);
		success &= Utilities::readHeaderVariable(fileContent, "versionPatch", librarySettings.version.patch);

		QString name;
		success &= Utilities::readHeaderVariable(fileContent, "name", name);
		success &= Utilities::readHeaderVariable(fileContent, "author", librarySettings.author);
		success &= Utilities::readHeaderVariable(fileContent, "email", librarySettings.email);
		success &= Utilities::readHeaderVariable(fileContent, "website", librarySettings.website);
		success &= Utilities::readHeaderVariable(fileContent, "license", librarySettings.license);

		if (name != cmakeSettings.libraryName)
		{
			QMessageBox::critical(0, "Error", "Library name in " + header + " does not match the library name in CMakeLists.txt");
			//return false;
		}
		settings.setLibrarySettings(librarySettings);
		settings.setCMAKE_settings(cmakeSettings);
		return success;
	}
	bool ProjectExporter::readDependencies(ProjectSettings& settings, const QString& projectDirPath)
	{
		QVector<QString> fileList = Utilities::getFilesInFolder(projectDirPath + "/dependencies", ".cmake");
		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		QVector<Dependency> dependencies;
		for (const auto& file : fileList)
		{
			Dependency dep;
			dep.setName(QFileInfo(file).baseName());
			dependencies.push_back(dep);
		}
		cmakeSettings.dependencies = dependencies;
		settings.setCMAKE_settings(cmakeSettings);
		return true;
	}

	

}