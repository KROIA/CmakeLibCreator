#include "ProjectExporter.h"
#include "Resources.h"
#include <QDir>
#include "Utilities.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QSet>
#include <QFile>

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
	Log::LogObject& ProjectExporter::getLogger()
	{
		static Log::LogObject logger("ProjectExporter");
		return logger;
	}

	bool ProjectExporter::exportProject(
		const ProjectSettings& settings,
		const QString& projectDirPath,
		const ExportSettings& expSettings)
	{
		QString normalizedPath = projectDirPath;
		normalizedPath = normalizedPath.replace("\\", "/");
		return instance().exportProject_intern(settings, normalizedPath, expSettings);
	}
	bool ProjectExporter::readProjectData(ProjectSettings& settings, const QString& projectDirPath)
	{
		QString normalizedPath = projectDirPath;
		normalizedPath = normalizedPath.replace("\\", "/");
		bool ret = instance().readProjectData_intern(settings, normalizedPath);
		settings = settings.getValidated();
		return ret;
	}

	bool ProjectExporter::exportProject_intern(
		ProjectSettings settings,
		const QString& projectDirPath,
		const ExportSettings& expSettings)
	{
		bool success = true;
		settings.setDefaultPlaceholder(ProjectSettings::s_defaultPlaceholder);

		// LIBRARY_VERSION string is the write-side master for CMakeLists.txt.
		// The UI stores version as int fields in LibrarySettings::version; always
		// format them into the string so replaceTemplateVariablesIn_mainCmakeLists
		// writes the correct value even when creating a brand-new project.
		{
			ProjectSettings::CMAKE_settings cms = settings.getCMAKE_settings();
			const ProjectSettings::LibrarySettings& ls = settings.getLibrarySettings();
			cms.libraryVersion = QString("%1.%2.%3")
				.arg(ls.version.major)
				.arg(ls.version.minor)
				.arg(ls.version.patch);
			settings.setCMAKE_settings(cms);
		}
		//const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		//const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();
		
		// Capture developer-added macros before the template overwrites the project's
		// CMakePresets.json / CMakeSettings.json. Only relevant on upgrade (project files
		// already exist). Re-applied after placeholder substitution. Template wins on
		// any key the template manages.
		CmakeMacroSnapshot userMacroSnapshot;
		const bool isUpgradePath = !expSettings.copyAllTemplateFiles && expSettings.replaceTemplateCmakeFiles;
		if (isUpgradePath)
		{
			userMacroSnapshot = snapshotUserCmakeMacros(projectDirPath);
		}

		if (expSettings.copyAllTemplateFiles)
		{
			// Create the project directory
			QDir dir;
			QDir projectDir(projectDirPath);
			if (dir.exists(projectDirPath))
			{
				success &= Utilities::deleteFolderRecursively(projectDirPath);
			}

			if (!dir.mkpath(projectDirPath))
			{
				Utilities::critical("Error", "Failed to create project directory:\n" + projectDirPath);
				return false;
			}

			success &= copyTemplateSourceFiles(settings, projectDirPath,
				{
					"core",
					"examples",
					"unittests",
				},{""});
			

			success &= replaceTemplateUserSectionsIn_codeFiles(settings, projectDirPath);
			success &= copyTemplateLibraryFiles(settings, projectDirPath);
			success &= replaceTemplateUserSectionsIn_cmakeLists(settings, projectDirPath);
		}
		else
		{
			if (expSettings.replaceTemplateCodeFiles)
			{
				//ProjectSettings::Placeholder placeholder = settings.getPlaceholder();
				//placeholder.Library_Namespace = ProjectSettings::s_defaultPlaceholder.Library_Namespace;
				//placeholder.Library_Name = ProjectSettings::s_defaultPlaceholder.Library_Name;
				//settings.setPlaceholder(placeholder);

				success &= copyTemplateSourceFiles(settings, projectDirPath,
					{
						"core",
						//"examples",
						//"unittests",
					}, {".h",".cpp"});
				success &= replaceTemplateUserSectionsIn_codeFiles(settings, projectDirPath);
			}
			if (expSettings.replaceTemplateCmakeFiles)
			{
				//ProjectSettings::Placeholder placeholder = settings.getPlaceholder();
				//placeholder.LIBRARY__NAME_EXPORT = ProjectSettings::s_defaultPlaceholder.LIBRARY__NAME_EXPORT;
				//placeholder.LIBRARY__NAME_LIB = ProjectSettings::s_defaultPlaceholder.LIBRARY__NAME_LIB;
				//placeholder.LIBRARY__NAME_SHORT = ProjectSettings::s_defaultPlaceholder.LIBRARY__NAME_SHORT;
				//settings.setPlaceholder(placeholder);
				success &= copyTemplateLibraryFiles(settings, projectDirPath);
				success &= replaceTemplateUserSectionsIn_cmakeLists(settings, projectDirPath);
			}
		}
		
		success &= copyTemplateDependencies(settings, projectDirPath);
		success &= replaceTemplateFileNames(settings, projectDirPath);
		if (expSettings.replaceTemplateVariables)
		{
			success &= replaceTemplateVariables(settings, projectDirPath);
		}
		if (expSettings.replaceTemplateCodePlaceholders)
		{
			success &= replaceTemplateCodePlaceholders(settings,
				{
					"core",
					"examples",
					"unittests"
				}, projectDirPath);
		}

		if (isUpgradePath && userMacroSnapshot.valid)
		{
			success &= applyUserCmakeMacros(userMacroSnapshot, projectDirPath);
		}

		return success;
	}
	bool ProjectExporter::readProjectData_intern(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		success &= readCmakeLists(settings, projectDirPath);
		success &= readLibraryInfo(settings, projectDirPath);
		success &= readDependencies(settings, projectDirPath);
		success &= readCmakeUserSections(settings, projectDirPath);
		success &= readCodeUserSections(settings, projectDirPath);



		ProjectSettings::Placeholder placeholder;
		const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();
		placeholder.Library_Namespace = librarySettings.namespaceName;
		placeholder.LIBRARY__NAME_API = librarySettings.apiName;
		placeholder.Library_Name = cmakeSettings.libraryName;
		placeholder.LIBRARY__NAME_LIB = cmakeSettings.lib_define;
		placeholder.LIBRARY__NAME_SHORT = cmakeSettings.lib_short_define;
		settings.setLoadedPlaceholder(placeholder);
		return success;
	}

	bool ProjectExporter::copyTemplateLibraryFiles(
		const ProjectSettings& settings, 
		const QString& projectDirPath)
	{
		CLC_UNUSED(settings);
		bool success = true;
		QString templateSourcePath = Resources::getCurrentTemplateAbsSourcePath();
		QVector<QString> sourceDirs{
			"cmake",
		};

		for (const auto& sourceDir : sourceDirs)
		{
			QString source = templateSourcePath + "/" + sourceDir;
			QString target = projectDirPath + "/" + sourceDir;
			success &= Utilities::copyAndReplaceFolderContents(source, target);
		}

		QVector<QString> fileList{
			//templateSourcePath + "/.gitignore",
			templateSourcePath + "/CMakeSettings.json",
			templateSourcePath + "/CMakePresets.json",
			templateSourcePath + "/build.bat",
			templateSourcePath + "/CMakeLists.txt",
		};
		fileList += Utilities::getFilesInFolderRecursive(templateSourcePath+"/core", "CMakeLists.txt");
		//fileList += Utilities::getFilesInFolderRecursive(templateSourcePath+"/examples", "CMakeLists.txt");
		//fileList += Utilities::getFilesInFolderRecursive(templateSourcePath+"/unitTests", "CMakeLists.txt");

		QString destinationExampleBasePath = projectDirPath + "/examples";
		QString destinationUnitTestBasePath = projectDirPath + "/unitTests";
		QVector<QString> exampleFiles = Utilities::getFilesInFolderRecursive(destinationExampleBasePath, "CMakeLists.txt");
		QVector<QString> unitTestFiles = Utilities::getFilesInFolderRecursive(destinationUnitTestBasePath, "CMakeLists.txt");

		//const QVector<ProjectSettings::CMakeFileUserSections>&  userSections = settings.getCmakeUserSections();
		for (const auto& source : fileList)
		{
			QString destination = projectDirPath + "/" + source.mid(source.indexOf(templateSourcePath)+ templateSourcePath.size()+1);
			if (!Utilities::copyFile(source, destination, true))
				Utilities::critical("Error", "Failed to copy file:\n" + source);
		}

		for (const auto& destination : exampleFiles)
		{
			QString subDir = destination.mid(destination.indexOf(destinationExampleBasePath) + destinationExampleBasePath.size() + 1);
			QString source = templateSourcePath + "/examples/" + subDir;
			if(subDir.indexOf("/") != -1 || subDir.indexOf("\\") != -1)
				source = templateSourcePath + "/examples/LibraryExample/CMakeLists.txt";

			if (!Utilities::copyFile(source, destination, true))
				Utilities::critical("Error", "Failed to copy file:\n" + source);
		}
		for (const auto& destination : unitTestFiles)
		{
			QString subDir = destination.mid(destination.indexOf(destinationUnitTestBasePath) + destinationUnitTestBasePath.size() + 1);
			QString source = templateSourcePath + "/unitTests/" + subDir;
			if (subDir.indexOf("/") != -1 || subDir.indexOf("\\") != -1)
				source = templateSourcePath + "/unitTests/ExampleTest/CMakeLists.txt";

			if (!Utilities::copyFile(source, destination, true))
				Utilities::critical("Error", "Failed to copy file:\n" + source);
		}

		/*QVector<QPair<QString, QString>> cmakeFiles = {
			{templateSourcePath + "/CMakeLists.txt", },
			templateSourcePath + "/core/CMakeLists.txt"
		};*/

		// override the core CMakeLists.txt
		/*if (!Utilities::copyFile(templateSourcePath + "/core/CMakeLists.txt", projectDirPath + "/core/CMakeLists.txt", true))
		{
			Utilities::critical("Error", "Failed to copy file:\n" + templateSourcePath + "/core/CMakeLists.txt");
		}*/
		

		return success;
	}
	bool ProjectExporter::copyTemplateSourceFiles(
		const ProjectSettings& settings,
		const QString& projectDirPath,
		const QVector<QString>& sourceDirs,
		const QVector<QString>& filters)
	{
		CLC_UNUSED(settings);
		CLC_UNUSED(projectDirPath);
		bool success = true;
		
		QString templateName = ProjectSettings::s_defaultPlaceholder.Library_Name;
		QString targetName = settings.getCMAKE_settings().libraryName;
		QString templateSourcePath = Resources::getCurrentTemplateAbsSourcePath();
		for (const auto& sourceDir : sourceDirs)
		{
			QVector<QString> fileList;
			for (int i = 0; i < filters.size(); ++i)
				fileList += Utilities::getFilesInFolderRecursive(templateSourcePath + "/" + sourceDir, filters[i]);
			for (auto file : fileList)
			{
				QString relativePath = file.mid(file.indexOf(templateSourcePath) + templateSourcePath.size() + 1);
				QString source = file;
				QString target = relativePath;
				// Get file name
				QString fileName = QFileInfo(file).fileName();
				if (fileName.contains(templateName))
				{

					target = target.replace(templateName, targetName);
				}
				target = projectDirPath + "/" + target;
				if (!Utilities::copyFile(source, target, true))
				{
					Utilities::critical("Error", "Failed to copy file:\n" + source);
				}
			}

			/*
			QString source = templateSourcePath + "/" + sourceDir;
			QString target = projectDirPath + "/" + sourceDir;
			success &= Utilities::copyAndReplaceFolderContents(source, target);*/
		}

		return success;
	}
	bool ProjectExporter::copyTemplateDependencies(const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		bool success = true;
		QString depsPath = Resources::getDependenciesAbsSourcePath();
		QString templateSourcePath = Resources::getCurrentTemplateAbsSourcePath();
		
		if (!Utilities::createFolder(projectDirPath + "/dependencies"))
		{
			Utilities::critical("Error", "Failed to create folder:\n" + depsPath);
			return false;
		}
		if (!Utilities::copyFile(templateSourcePath+"/dependencies/.gitignore", projectDirPath + "/dependencies/.gitignore", true))
		{
			Utilities::critical("Error", "Failed to copy file:\n" + depsPath + "/.gitignore");
			return false;
		}

		//const QVector<Dependency> &possibleDeps = Resources::getDependencies();
		// Remove all existing dependencies if they are official dependencies from the manager
		const QVector<Dependency> &dependencies = settings.getCMAKE_settings().dependencies;
		QVector<QString> existingDeps = Utilities::getFilesInFolder(projectDirPath + "/dependencies/", ".cmake");
		// Search for the "order.cmake" and remove it from the list
		for (int i = 0; i < existingDeps.size(); ++i)
		{
			if (existingDeps[i].indexOf("order.cmake") != -1)
			{
				existingDeps.remove(i);
				break;
			}
		}
		for (const auto& dep : existingDeps)
		{
			
			QString depFileName = QFileInfo(dep).fileName();
			if (dep.indexOf(".cmake") != -1)
				depFileName = depFileName.mid(0, depFileName.indexOf(".cmake"));
			
			if (!Resources::isOriginalDependency(depFileName))
			{
				// Remove it if it is no longer selected
				bool found = false;
				for (const auto& selectedDep : dependencies)
				{
					if (selectedDep.getName() == depFileName)
					{
						found = true;
						break;
					}
				}
				if(!found)
					success &= Utilities::deleteFile(dep);
			}
			else
				success &= Utilities::deleteFile(dep);
		}
		// Remove cache
		QVector<QString> cachePaths = Utilities::getFoldersInFolder(projectDirPath + "/dependencies/");
		for (const auto& cache : cachePaths)
		{
			success &= Utilities::deleteFolderRecursively(cache);
		}

		for (const auto& dep : dependencies)
		{
			if (!Resources::isOriginalDependency(dep.getName()))
				continue;
			QString depFileName = dep.getName() + ".cmake";
			QString source = depsPath + "/" + depFileName;
			QString target = projectDirPath + "/dependencies/" + depFileName;
			if (!Utilities::copyFile(source, target, true))
			{
				Utilities::critical("Error", "Failed to copy file:\n" + source);
				success = false;
			}
		}

		// Copy the unittest library dependency file to the unittest folder
		{
			QString source = depsPath + "/UnitTest.cmake";
			if (QFile::exists(source))
			{
				QString target = projectDirPath + "/unittests/UnitTest.cmake";
				if (!Utilities::copyFile(source, target, true))
				{
					Utilities::critical("Error", "Failed to copy file:\n" + source);
					success = false;
				}
			}
		}
		return success;
	}

	bool ProjectExporter::replaceTemplateVariables(const ProjectSettings& settings,
												   const QString& projectDirPath)
	{
		bool success = true;
		success &= replaceTemplateVariablesIn_mainCmakeLists(settings, projectDirPath);
		success &= replaceTemplateVariablesIn_cmakeSettings(settings, projectDirPath);
		success &= replaceTemplateVariablesIn_cmakePresets(settings, projectDirPath);
		success &= replaceTemplateVariablesIn_libraryInfo(settings, projectDirPath);

		return success;
	}
	bool ProjectExporter::replaceTemplateFileNames(
		const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		QString targetFileNameContains = settings.getDefaultPlaceholder().Library_Name;
		QString libraryName = settings.getCMAKE_settings().libraryName;
		if(targetFileNameContains == libraryName)
			return true; // nothing to do

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
				newFileName.replace(targetFileNameContains, libraryName);
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
			Utilities::critical("Error", "Failed to read file:\n"+ cmakeListsPath);
			return false;
		}

		//const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();

		// Replace the library name
		success &= Utilities::replaceCmakeVariable(fileContent, "LIBRARY_NAME", cmakeSettings.libraryName);
		success &= Utilities::replaceCmakeVariableString(fileContent, "LIBRARY_VERSION", cmakeSettings.libraryVersion);
		success &= Utilities::replaceCmakeVariable(fileContent, "LIB_DEFINE", cmakeSettings.lib_define);
		success &= Utilities::replaceCmakeVariable(fileContent, "LIB_PROFILE_DEFINE", cmakeSettings.lib_profile_define);

		success &= Utilities::replaceCmakeVariable(fileContent, "QT_ENABLE", (cmakeSettings.qt_enable?"ON":"OFF"));
		success &= Utilities::replaceCmakeVariable(fileContent, "QT_DEPLOY", (cmakeSettings.qt_deploy?"ON":"OFF"));

		QVector<QString> qtModules;
		for (const auto& module : cmakeSettings.qModules)
		{
			qtModules.push_back(module.getName());
		}
		success &= Utilities::replaceCmakeVariable(fileContent, "QT_MODULES", qtModules);

		QVector<QString> customDefines;
		for (const auto& def : cmakeSettings.customDefines)
		{
			customDefines.push_back(def);
		}
		success &= Utilities::replaceCmakeVariable(fileContent, "USER_SPECIFIC_DEFINES", customDefines);
		QVector<QString> customGlobalDefines;
		for (const auto& def : cmakeSettings.customGlobalDefines)
		{
			customGlobalDefines.push_back(def);
		}
		success &= Utilities::replaceCmakeVariable(fileContent, "USER_SPECIFIC_GLOBAL_DEFINES", customGlobalDefines);



		success &= Utilities::replaceCmakeVariable(fileContent, "DEBUG_POSTFIX_STR", cmakeSettings.debugPostFix);
		success &= Utilities::replaceCmakeVariable(fileContent, "STATIC_POSTFIX_STR", cmakeSettings.staticPostFix);
		success &= Utilities::replaceCmakeVariable(fileContent, "PROFILING_POSTFIX_STR", cmakeSettings.profilingPostFix);

		success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", QString::number(cmakeSettings.cxxStandard));
		success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", (cmakeSettings.cxxStandardRequired?"ON":"OFF"));

		success &= Utilities::replaceCmakeVariable(fileContent, "COMPILE_EXAMPLES", (cmakeSettings.compile_examples?"ON":"OFF"));
		success &= Utilities::replaceCmakeVariable(fileContent, "COMPILE_UNITTESTS", (cmakeSettings.compile_unittests?"ON":"OFF"));

		success &= Utilities::replaceCmakeVariableString(fileContent, "QT_INSTALL_BASE", cmakeSettings.qt_installBase);
		success &= Utilities::replaceCmakeVariable(fileContent, "QT_MAJOR_VERSION", QString::number(cmakeSettings.qt_versionNr[0]));
		success &= Utilities::replaceCmakeVariableString(fileContent, "QT_VERSION", cmakeSettings.getQtVersionStr());
		success &= Utilities::replaceCmakeVariableString(fileContent, "QT_COMPILER", cmakeSettings.qt_compiler);

		{
			QString target = "_NO_EXAMPLES";
			int line = Utilities::getLineIndex(fileContent, target, false, "#");
			if (line != -1)
			{
				QString lineContent = fileContent[line];
				// split by space
				QStringList list = lineContent.split(" ");
				for (int i = 0; i < list.size(); ++i)
				{
					if (list[i].contains(target))
					{
						list[i] = cmakeSettings.libraryName + target + list[i].mid(list[i].indexOf(target)+ target.size());
						break;
					}
				}
				fileContent[line] = list.join(" ");
			}
		}

		{
			QString target = "_NO_UNITTESTS";
			int line = Utilities::getLineIndex(fileContent, target, false, "#");
			if (line != -1)
			{
				QString lineContent = fileContent[line];
				// split by space
				QStringList list = lineContent.split(" ");
				for (int i = 0; i < list.size(); ++i)
				{
					if (list[i].contains(target))
					{
						list[i] = cmakeSettings.libraryName + target + list[i].mid(list[i].indexOf(target) + target.size());
						break;
					}
				}
				fileContent[line] = list.join(" ");
			}
		}

		success &= Utilities::saveFileContents(cmakeListsPath, fileContent);

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
			Utilities::critical("Error", "Failed to read file:\n" + cmakeListsPath);
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
			Utilities::critical("Error", "Failed to write file:\n" + cmakeListsPath);
			return false;
		}
		file.write(doc.toJson());
		file.close();
		return true;
	}

	bool ProjectExporter::replaceTemplateVariablesIn_cmakePresets(const ProjectSettings& settings,
																  const QString& projectDirPath)
	{
		QString presetsPath = projectDirPath + "/CMakePresets.json";
		if (!QFile::exists(presetsPath))
			return true; // optional file in older projects

		QVector<QString> fileContent = Utilities::getFileContents(presetsPath);
		if (fileContent.size() == 0)
		{
			getLogger().logError("Failed to read file:\n" + presetsPath.toStdString());
			Utilities::critical("Error", "Failed to read file:\n" + presetsPath);
			return false;
		}

		const ProjectSettings::LibrarySettings& librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings& cmakeSettings = settings.getCMAKE_settings();
		const ProjectSettings::Placeholder defaults = settings.getDefaultPlaceholder();

		struct Replacement { QString from; QString to; };
		// Order matters: longer / more-specific tokens before tokens that are substrings of them
		// (e.g. LIBRARY_NAME_SHORT must run before LIBRARY_NAME_LIB / LIBRARY_NAME_API would,
		// and Library_Name must run after LIBRARY_NAME_* so it doesn't eat the prefix).
		const QVector<Replacement> replacements{
			{ defaults.LIBRARY__NAME_SHORT, cmakeSettings.lib_short_define },
			{ defaults.LIBRARY__NAME_API,   librarySettings.apiName        },
			{ defaults.LIBRARY__NAME_LIB,   cmakeSettings.lib_define       },
			{ defaults.Library_Namespace,   librarySettings.namespaceName  },
			{ defaults.Library_Name,        cmakeSettings.libraryName      },
		};

		bool changed = false;
		for (auto& line : fileContent)
		{
			for (const auto& r : replacements)
			{
				if (r.from.isEmpty() || r.from == r.to)
					continue;
				if (line.contains(r.from))
				{
					line.replace(r.from, r.to);
					changed = true;
				}
			}
		}

		if (!changed)
			return true;

		if (!Utilities::saveFileContents(presetsPath, fileContent))
		{
			getLogger().logError("Failed to write file:\n" + presetsPath.toStdString());
			Utilities::critical("Error", "Failed to write file:\n" + presetsPath);
			return false;
		}
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
			Utilities::critical("Error", "Failed to read file:\n" + header);
			return false;
		}

		const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings& cmakeSettings = settings.getCMAKE_settings();

		// Version and name are now macro references driven by CMakeLists.txt (new-style projects
		// that ship a _meta.h.in file).  Writing literal values would overwrite those references.
		// For legacy projects without _meta.h.in the string literals are still written directly.
		bool isNewStyle = QFile::exists(projectDirPath + "/core/inc/" + cmakeSettings.libraryName + "_meta.h.in");
		if (!isNewStyle)
		{
			if(success) success &= Utilities::replaceHeaderVariable(fileContent, "name", "\"" + cmakeSettings.libraryName + "\"");
		}
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "author", "\"" + librarySettings.author + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "email", "\"" + librarySettings.email + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "website", "\"" + librarySettings.website + "\"");
		if(success) success &= Utilities::replaceHeaderVariable(fileContent, "license", "\"" + librarySettings.license + "\"");



		if(success) success &= Utilities::saveFileContents(header, fileContent);

		return success;
	}

	bool ProjectExporter::replaceTemplateCodePlaceholders(
		const ProjectSettings& settings,
		const QVector<QString>& sourceDirs,
		const QString& projectDirPath)
	{
		struct File
		{
			QString path;
			QVector<QString> data;
		};
		QVector<File> files;

		QVector<QString> fileList;
		for (int i = 0; i < sourceDirs.size(); ++i)
		{
			fileList += Utilities::getFilesInFolderRecursive(projectDirPath+"/"+sourceDirs[i], ".h");
			fileList += Utilities::getFilesInFolderRecursive(projectDirPath+"/"+sourceDirs[i], ".cpp");
		} 
		for (const auto& file : fileList)
		{
			File f;
			f.path = file;
			f.data = Utilities::getFileContents(file);
			files.push_back(f);
		}
		const ProjectSettings::LibrarySettings& librarySettigns = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings& cmakeSettings = settings.getCMAKE_settings();
		
		ProjectSettings::Placeholder defaultPlaceholders = settings.getDefaultPlaceholder();
		ProjectSettings::Placeholder loadedPlaceholders = settings.getLoadedPlaceholder();
		struct Replacements
		{
			QString from;
			QString to;
			QVector<QVector<QString>> mustContainInLine;
		};
		QVector<Replacements> replacements{
			{defaultPlaceholders.Library_Namespace,librarySettigns.namespaceName,	{}},
			{defaultPlaceholders.LIBRARY__NAME_API,librarySettigns.apiName,  {{defaultPlaceholders.LIBRARY__NAME_API + " "}, {"#","define"}}},
			{defaultPlaceholders.LIBRARY__NAME_SHORT,cmakeSettings.lib_short_define, {}},
			{defaultPlaceholders.LIBRARY__NAME_LIB,cmakeSettings.lib_define, {{"#"}}},
			// Replace CmakeLibraryCreator on #include lines, version-macro lines, name-macro lines, and Doxygen @file comment lines.
			{defaultPlaceholders.Library_Name, cmakeSettings.libraryName, {{"#include"}, {"_VERSION_"}, {"_LIBRARY_NAME"}, {"@file"}}},

			//{loadedPlaceholders.Library_Namespace,librarySettigns.namespaceName,	{}},
			//{loadedPlaceholders.LIBRARY__NAME_EXPORT,librarySettigns.exportName,  {{loadedPlaceholders.LIBRARY__NAME_EXPORT + " "}, {"#","define"}}},
			//{loadedPlaceholders.LIBRARY__NAME_SHORT,cmakeSettings.lib_short_define, {}},
			//{loadedPlaceholders.LIBRARY__NAME_LIB,cmakeSettings.lib_define, {{"#"}}},
			//{loadedPlaceholders.Library_Name ,cmakeSettings.libraryName, {{"#include"}}}
		};

		for (auto& file : files)
		{
			bool hasChanges = false;
			for (const auto& replacement : replacements)
			{
				hasChanges |= Utilities::replaceAllIfLineContains(file.data, replacement.from, replacement.to, replacement.mustContainInLine);
			}
			if(hasChanges)
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
		const QVector<ProjectSettings::CMakeFileUserSections> &userSections = settings.getCmakeUserSections();
		bool success = true;
		for (const ProjectSettings::CMakeFileUserSections& sectionList : userSections)
		{
			// Check if the file exists
			if (!QFile::exists(sectionList.file))
			{
				Utilities::critical("Error", "Failed to find file:\n" + sectionList.file+"\nto replace user sections");
				success &= false;
				continue;
			}
			success &= Utilities::replaceUserCmakeSections(sectionList.file, sectionList.sections);
		}
		return success;
	}

	bool ProjectExporter::replaceTemplateUserSectionsIn_codeFiles(
		const ProjectSettings& settings,
		const QString& projectDirPath)
	{
		CLC_UNUSED(projectDirPath);
		//QVector<QString> smakeListsFiles = Utilities::getFilesInFolderRecursive(projectDirPath, "CMakeLists.txt");
		const QVector<ProjectSettings::CodeUserSections>& userSections = settings.getCodeUserSections();
		bool success = true;
		for (const ProjectSettings::CodeUserSections& sectionList : userSections)
		{
			// Check if the file exists
			if (!QFile::exists(sectionList.file))
			{
				getLogger().logError("Failed to find file:\n" + sectionList.file.toStdString() + "\nto replace user sections");
				Utilities::critical("Error", "Failed to find file:\n" + sectionList.file + "\nto replace user sections");
				success &= false;
				continue;
			}
			success &= Utilities::replaceUserCodeSections(sectionList.file, sectionList.sections);
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
			Utilities::critical("Error", "Failed to read file:\n" + cmakeListsPath);
			return false;
		}
	}*/

	bool ProjectExporter::readCmakeLists(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		QString cmakeListsPath = projectDirPath + "/CMakeLists.txt";
		if (QFile::exists(cmakeListsPath) == false)
		{
			Utilities::critical("Error", "Failed to find file:\n" + cmakeListsPath+"\nIs this a library directory?");
			return false;
		}
		QVector<QString> fileContent = Utilities::getFileContents(cmakeListsPath);
		if (fileContent.size() == 0)
		{
			Utilities::critical("Error", "Failed to read file:\n" + cmakeListsPath);
			return false;
		}

		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		success &= Utilities::readCmakeVariable(fileContent, "LIBRARY_NAME", cmakeSettings.libraryName);
		Utilities::readCmakeVariableString(fileContent, "LIBRARY_VERSION", cmakeSettings.libraryVersion); // optional — not present in older projects
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

		QVector<QString> customDefines;
		success &= Utilities::readCmakeVariables(fileContent, "USER_SPECIFIC_DEFINES", customDefines);
		cmakeSettings.customDefines.clear();
		for(auto def : customDefines)
		{
			QStringList splitted = def.split(" ");
			// Remove whitespaces and empty strings
			for(int i = 0; i < splitted.size(); ++i)
			{
				splitted[i] = splitted[i].trimmed();
				if(splitted[i].isEmpty())
				{
					splitted.removeAt(i);
					i--;
				}
				else
				{
					cmakeSettings.customDefines.push_back(splitted[i]);
				}
			}
		}

		QVector<QString> customGlobalDefines;
		success &= Utilities::readCmakeVariables(fileContent, "USER_SPECIFIC_GLOBAL_DEFINES", customGlobalDefines);
		cmakeSettings.customGlobalDefines.clear();
		for (auto def : customGlobalDefines)
		{
			QStringList splitted = def.split(" ");
			// Remove whitespaces and empty strings
			for (int i = 0; i < splitted.size(); ++i)
			{
				splitted[i] = splitted[i].trimmed();
				if (splitted[i].isEmpty())
				{
					splitted.removeAt(i);
					i--;
				}
				else
				{
					cmakeSettings.customGlobalDefines.push_back(splitted[i]);
				}
			}
		}
		


		success &= Utilities::readCmakeVariable(fileContent, "DEBUG_POSTFIX_STR", cmakeSettings.debugPostFix);
		success &= Utilities::readCmakeVariable(fileContent, "STATIC_POSTFIX_STR", cmakeSettings.staticPostFix);
		success &= Utilities::readCmakeVariable(fileContent, "PROFILING_POSTFIX_STR", cmakeSettings.profilingPostFix);


		success &= Utilities::readCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", cmakeSettings.cxxStandard);
		success &= Utilities::readCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", cmakeSettings.cxxStandardRequired);

		//if (success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD", QString::number(cmakeSettings.cxxStandard));
		//if (success) success &= Utilities::replaceCmakeVariable(fileContent, "CMAKE_CXX_STANDARD_REQUIRED", (cmakeSettings.cxxStandardRequired ? "ON" : "OFF"));

		success &= Utilities::readCmakeVariable(fileContent, "COMPILE_EXAMPLES", cmakeSettings.compile_examples);
		success &= Utilities::readCmakeVariable(fileContent, "COMPILE_UNITTESTS", cmakeSettings.compile_unittests);

		QString qtInstallBase;
		success &= Utilities::readCmakeVariableString(fileContent, "QT_INSTALL_BASE", qtInstallBase);
		cmakeSettings.setQtInstallBase(qtInstallBase);

		success &= Utilities::readCmakeVariable(fileContent, "QT_MAJOR_VERSION", cmakeSettings.qt_versionNr[0]);
		QString qtVersionStr;
		success &= Utilities::readCmakeVariableString(fileContent, "QT_VERSION", qtVersionStr);
		cmakeSettings.setQtVersion(qtVersionStr);

		QString qtCompilerStr;
		success &= Utilities::readCmakeVariableString(fileContent, "QT_COMPILER", qtCompilerStr);
		cmakeSettings.setQtCompiler(qtCompilerStr);

		settings.setCMAKE_settings(cmakeSettings);

		

		return success;
	}
	bool ProjectExporter::readLibraryInfo(ProjectSettings& settings, const QString& projectDirPath)
	{
		bool success = true;
		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		ProjectSettings::LibrarySettings librarySettings = settings.getLibrarySettings();

		// Read CLC from the header file
		{
			QString header = projectDirPath + "/core/inc/" + cmakeSettings.libraryName + "_global.h";
			QVector<QString> globalContent = Utilities::getFileContents(header);
			QString shortNameKey = "<LIBRARY NAME SHORT>=";
			int shortNameIndex = Utilities::getLineIndex(globalContent, shortNameKey, false, "//");
			if (shortNameIndex == -1)
			{
				// Alternative method to read the short name
				// Searchinf for line:
				// #define CLC_UNUSED(x) (void)x;
				shortNameIndex = Utilities::getLineIndex(globalContent, "_UNUSED(x)", false, "//");
				if (shortNameIndex != -1)
				{
					QString line = globalContent[shortNameIndex];
					if (line.indexOf("#define") != -1)
					{
						int start = line.indexOf("#define") + 7;
						int end = line.indexOf("_UNUSED(x)");
						line = line.mid(start, end - start);
						cmakeSettings.lib_short_define = line.trimmed();
					}
				}
			}
			else
			{
				QString line = globalContent[shortNameIndex];
				cmakeSettings.lib_short_define = line.mid(line.indexOf(shortNameKey) + shortNameKey.size()).trimmed();
			}
		}

		QString header = projectDirPath + "/core/inc/" + cmakeSettings.libraryName + "_info.h";
		QVector<QString> fileContent = Utilities::getFileContents(header);
		if (fileContent.size() == 0)
		{
			Utilities::critical("Error", "Failed to read file:\n" + header);
			return false;
		}

		// Read the namespace from that file
		QString namespaceKey = "namespace";
		int namespaceLineIndex = Utilities::getLineIndex(fileContent, namespaceKey, true, "//");
		if (namespaceLineIndex == -1)
		{
			Utilities::critical("Error", "Could not find namespace in " + header);
			//return false;
		}
		QString namespaceName = fileContent[namespaceLineIndex].mid(fileContent[namespaceLineIndex].indexOf(namespaceKey)+ namespaceKey.size()+1).trimmed();
		librarySettings.namespaceName = namespaceName;

		// Read the export name from that file
		int exportLineIndex1 = Utilities::getLineIndex(fileContent, "_API", false, "//");
		if(exportLineIndex1 == -1)
			exportLineIndex1 = Utilities::getLineIndex(fileContent, "EXPORT", false, "//");
		int exportLineIndex2 = Utilities::getLineIndex(fileContent, { "class","LibraryInfo" }, true, "//");
		if ((exportLineIndex1 == -1 || exportLineIndex2 == -1) || (exportLineIndex1 != exportLineIndex2))
		{
			Utilities::critical("Error", "Could not find api name in " + header);
			//return false;
		}
		else
		{
			QString line = fileContent[exportLineIndex1];
			QStringList splitted = line.split(" ");
			QString apiName;
			for (int i = 0; i < splitted.size(); ++i)
			{
				if (splitted[i].indexOf("EXPORT") != -1 || splitted[i].indexOf("API") != -1)
				{
					apiName = splitted[i].trimmed();
					break;
				}
			}
			
			
			librarySettings.apiName = apiName;
		}

		// Prefer LIBRARY_VERSION from CMakeLists.txt (canonical in new-style projects where
		// _info.h holds macro references instead of integer literals).
		// Fall back to reading integers from _info.h for old-style projects.
		if (!cmakeSettings.libraryVersion.isEmpty()) {
			QStringList vparts = cmakeSettings.libraryVersion.split(".");
			librarySettings.version.major = (vparts.size() > 0) ? vparts[0].trimmed().toInt() : 0;
			librarySettings.version.minor = (vparts.size() > 1) ? vparts[1].trimmed().toInt() : 0;
			librarySettings.version.patch = (vparts.size() > 2) ? vparts[2].trimmed().toInt() : 0;
		} else {
			success &= Utilities::readHeaderVariable(fileContent, "versionMajor", librarySettings.version.major);
			success &= Utilities::readHeaderVariable(fileContent, "versionMinor", librarySettings.version.minor);
			success &= Utilities::readHeaderVariable(fileContent, "versionPatch", librarySettings.version.patch);
		}

		// In new-style projects the name field is a macro reference (CmakeLibraryCreator_LIBRARY_NAME),
		// not a string literal.  The library name is already authoritative in cmakeSettings
		// (read from CMakeLists.txt), so we skip the string-equality check for those projects.
		bool isNewStyleRead = QFile::exists(projectDirPath + "/core/inc/" + cmakeSettings.libraryName + "_meta.h.in");
		if (!isNewStyleRead)
		{
			QString name;
			if (Utilities::readHeaderVariable(fileContent, "name", name) && name != cmakeSettings.libraryName)
				Utilities::critical("Error", "Library name in " + header + " does not match the library name in CMakeLists.txt");
		}

		success &= Utilities::readHeaderVariable(fileContent, "author", librarySettings.author);
		success &= Utilities::readHeaderVariable(fileContent, "email", librarySettings.email);
		success &= Utilities::readHeaderVariable(fileContent, "website", librarySettings.website);
		success &= Utilities::readHeaderVariable(fileContent, "license", librarySettings.license);
		settings.setLibrarySettings(librarySettings);
		settings.setCMAKE_settings(cmakeSettings);
		return success;
	}
	bool ProjectExporter::readDependencies(ProjectSettings& settings, const QString& projectDirPath)
	{
		QVector<QString> fileList = Utilities::getFilesInFolder(projectDirPath + "/dependencies", ".cmake");
		// Search for the file "order.cmake" and remove it if it was found
		for (int i = 0; i < fileList.size(); ++i)
		{
			if (fileList[i].indexOf("order.cmake") != -1)
			{
				fileList.removeAt(i);
				break;
			}
		}

		ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		QVector<Dependency> dependencies;
		for (const auto& file : fileList)
		{
			Dependency dep;
			dep.loadFromCmakeFile(file);
			dependencies.push_back(dep);
		}
		cmakeSettings.dependencies = dependencies;
		settings.setCMAKE_settings(cmakeSettings);
		return true;
	}

	bool ProjectExporter::readCmakeUserSections(ProjectSettings& settings, const QString& projectDirPath)
	{
		// Read User sections
		QVector<QString> sourceDirs{
			"core",
			"examples",
			"unittests",
		};

		QVector<QString> cmakeListsFiles;
		for(const auto &dir : sourceDirs)
			cmakeListsFiles  += Utilities::getFilesInFolderRecursive(projectDirPath+"/"+ dir, "CMakeLists.txt");
		cmakeListsFiles  += Utilities::getFilesInFolder(projectDirPath, "CMakeLists.txt");

		QVector<ProjectSettings::CMakeFileUserSections> userSections;
		for (const auto& file : cmakeListsFiles)
		{
			QVector<Utilities::UserSection> sections;
			Utilities::readUserCmakeSections(file, sections);
			if (sections.size() > 0)
			{
				ProjectSettings::CMakeFileUserSections userSection;
				userSection.file = file;
				userSection.sections = sections;
				userSections.push_back(userSection);
			}
		}
		settings.setCmakeUserSections(userSections);
		return true;
	}
	bool ProjectExporter::readCodeUserSections(ProjectSettings& settings, const QString& projectDirPath)
	{
		// Read User sections
		QVector<QString> sourceDirs{
			"core",
			"examples",
			"unittests",
		};

		QVector<QString> codeFiles;
		for (const auto& dir : sourceDirs)
		{
			codeFiles += Utilities::getFilesInFolderRecursive(projectDirPath + "/" + dir, ".h");
			codeFiles += Utilities::getFilesInFolderRecursive(projectDirPath + "/" + dir, ".cpp");
		}

		QVector<ProjectSettings::CodeUserSections> userSections;
		for (const auto& file : codeFiles)
		{
			QVector<Utilities::UserSection> sections;
			Utilities::readUserCodeSections(file, sections);
			if (sections.size() > 0)
			{
				ProjectSettings::CodeUserSections userSection;
				userSection.file = file;
				userSection.sections = sections;
				userSections.push_back(userSection);
			}
		}
		settings.setCodeUserSections(userSections);
		return true;
	}

	namespace
	{
		// Read a JSON file into a QJsonObject. Returns false if missing/unreadable/unparseable
		// or if the root is not an object. The caller decides how loud to be about failure.
		bool readJsonObject(const QString& path, QJsonObject& outObj)
		{
			QFile f(path);
			if (!f.exists() || !f.open(QIODevice::ReadOnly))
				return false;
			const QByteArray bytes = f.readAll();
			f.close();
			QJsonParseError err{};
			const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (doc.isNull() || !doc.isObject())
				return false;
			outObj = doc.object();
			return true;
		}

		bool writeJsonObject(const QString& path, const QJsonObject& obj)
		{
			QJsonDocument doc(obj);
			QFile f(path);
			if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
				return false;
			f.write(doc.toJson());
			f.close();
			return true;
		}

		// Tokenize a cmakeCommandArgs string. Whitespace splits tokens, except inside
		// double quotes — quoted segments stay attached to their token (quotes preserved
		// verbatim) so values like -DPATH="C:/Some Dir" survive round-trip.
		QStringList tokenizeCommandArgs(const QString& s)
		{
			QStringList tokens;
			QString cur;
			bool inQuotes = false;
			for (int i = 0; i < s.size(); ++i)
			{
				const QChar c = s[i];
				if (c == '"')
				{
					inQuotes = !inQuotes;
					cur.append(c);
				}
				else if (!inQuotes && c.isSpace())
				{
					if (!cur.isEmpty())
					{
						tokens.append(cur);
						cur.clear();
					}
				}
				else
				{
					cur.append(c);
				}
			}
			if (!cur.isEmpty())
				tokens.append(cur);
			return tokens;
		}

		// Returns "<KEY>" for tokens shaped like -D<KEY> or -D<KEY>=<VALUE>, else "".
		QString extractDefineKey(const QString& token)
		{
			if (!token.startsWith("-D"))
				return QString();
			QString rest = token.mid(2);
			const int eq = rest.indexOf('=');
			if (eq >= 0)
				rest = rest.left(eq);
			return rest.trimmed();
		}
	}

	ProjectExporter::CmakeMacroSnapshot ProjectExporter::snapshotUserCmakeMacros(const QString& projectDirPath) const
	{
		CmakeMacroSnapshot snap;
		const QString templateRoot = Resources::getCurrentTemplateAbsSourcePath();

		// --- CMakePresets.json ---
		{
			const QString projectPath  = projectDirPath + "/CMakePresets.json";
			const QString templatePath = templateRoot   + "/CMakePresets.json";
			QJsonObject projectObj, templateObj;
			const bool projectOk  = readJsonObject(projectPath, projectObj);
			const bool templateOk = readJsonObject(templatePath, templateObj);
			if (projectOk && templateOk)
			{
				const QJsonArray projectPresets  = projectObj.value("configurePresets").toArray();
				const QJsonArray templatePresets = templateObj.value("configurePresets").toArray();
				for (const QJsonValue& pVal : projectPresets)
				{
					const QJsonObject p = pVal.toObject();
					const QString name = p.value("name").toString();
					if (name.isEmpty())
						continue;
					// find matching template preset by name
					QJsonObject tplPreset;
					bool found = false;
					for (const QJsonValue& tVal : templatePresets)
					{
						const QJsonObject t = tVal.toObject();
						if (t.value("name").toString() == name)
						{
							tplPreset = t;
							found = true;
							break;
						}
					}
					if (!found)
						continue; // out of scope: drop unmatched presets entirely
					const QJsonObject projectCache  = p.value("cacheVariables").toObject();
					const QJsonObject templateCache = tplPreset.value("cacheVariables").toObject();
					QJsonObject extras;
					for (auto it = projectCache.begin(); it != projectCache.end(); ++it)
					{
						if (!templateCache.contains(it.key()))
							extras.insert(it.key(), it.value());
					}
					if (!extras.isEmpty())
						snap.presetExtras.insert(name, extras);
				}
			}
			else if (!projectOk && QFile::exists(projectPath))
			{
				getLogger().logWarning("Could not parse existing CMakePresets.json; user-macro preservation skipped for this file.");
			}
			else if (!templateOk)
			{
				getLogger().logWarning("Could not read template CMakePresets.json; user-macro preservation skipped for presets.");
			}
		}

		// --- CMakeSettings.json ---
		{
			const QString projectPath  = projectDirPath + "/CMakeSettings.json";
			const QString templatePath = templateRoot   + "/CMakeSettings.json";
			QJsonObject projectObj, templateObj;
			const bool projectOk  = readJsonObject(projectPath, projectObj);
			const bool templateOk = readJsonObject(templatePath, templateObj);
			if (projectOk && templateOk)
			{
				const QJsonArray projectConfigs  = projectObj.value("configurations").toArray();
				const QJsonArray templateConfigs = templateObj.value("configurations").toArray();
				for (const QJsonValue& cVal : projectConfigs)
				{
					const QJsonObject cfg = cVal.toObject();
					const QString name = cfg.value("name").toString();
					if (name.isEmpty())
						continue;
					QJsonObject tplCfg;
					bool found = false;
					for (const QJsonValue& tVal : templateConfigs)
					{
						const QJsonObject t = tVal.toObject();
						if (t.value("name").toString() == name)
						{
							tplCfg = t;
							found = true;
							break;
						}
					}
					if (!found)
						continue;

					// -D tokens
					const QStringList projectTokens  = tokenizeCommandArgs(cfg.value("cmakeCommandArgs").toString());
					const QStringList templateTokens = tokenizeCommandArgs(tplCfg.value("cmakeCommandArgs").toString());
					QSet<QString> templateKeys;
					for (const QString& tt : templateTokens)
					{
						const QString k = extractDefineKey(tt);
						if (!k.isEmpty())
							templateKeys.insert(k);
					}
					ConfigExtras extras;
					for (const QString& pt : projectTokens)
					{
						const QString k = extractDefineKey(pt);
						if (k.isEmpty())
							continue;
						if (!templateKeys.contains(k))
							extras.extraDFlags.append(pt);
					}

					// variables[]
					const QJsonArray projectVars  = cfg.value("variables").toArray();
					const QJsonArray templateVars = tplCfg.value("variables").toArray();
					QSet<QString> templateVarNames;
					for (const QJsonValue& tv : templateVars)
						templateVarNames.insert(tv.toObject().value("name").toString());
					for (const QJsonValue& pv : projectVars)
					{
						const QJsonObject pvo = pv.toObject();
						const QString vname = pvo.value("name").toString();
						if (vname.isEmpty())
							continue;
						if (!templateVarNames.contains(vname))
							extras.extraVariables.append(pvo);
					}

					if (!extras.extraDFlags.isEmpty() || !extras.extraVariables.isEmpty())
						snap.settingsExtras.insert(name, extras);
				}
			}
			else if (!projectOk && QFile::exists(projectPath))
			{
				getLogger().logWarning("Could not parse existing CMakeSettings.json; user-macro preservation skipped for this file.");
			}
			else if (!templateOk)
			{
				getLogger().logWarning("Could not read template CMakeSettings.json; user-macro preservation skipped for settings.");
			}
		}

		snap.valid = true;
		return snap;
	}

	bool ProjectExporter::applyUserCmakeMacros(const CmakeMacroSnapshot& snapshot, const QString& projectDirPath)
	{
		bool success = true;

		// --- CMakePresets.json ---
		if (!snapshot.presetExtras.isEmpty())
		{
			const QString path = projectDirPath + "/CMakePresets.json";
			QJsonObject root;
			if (!readJsonObject(path, root))
			{
				getLogger().logWarning("Skipping preset macro re-injection: cannot read " + path.toStdString());
			}
			else
			{
				QJsonArray presets = root.value("configurePresets").toArray();
				for (int i = 0; i < presets.size(); ++i)
				{
					QJsonObject p = presets[i].toObject();
					const QString name = p.value("name").toString();
					if (!snapshot.presetExtras.contains(name))
						continue;
					const QJsonObject extras = snapshot.presetExtras.value(name);
					QJsonObject cache = p.value("cacheVariables").toObject();
					for (auto it = extras.begin(); it != extras.end(); ++it)
					{
						// Template wins: if the post-substitution template re-introduced this key, keep template's value.
						if (cache.contains(it.key()))
							continue;
						cache.insert(it.key(), it.value());
						getLogger().logInfo("Preserved user cacheVariable '" + it.key().toStdString()
							+ "' in preset '" + name.toStdString() + "'");
					}
					p.insert("cacheVariables", cache);
					presets[i] = p;
				}
				root.insert("configurePresets", presets);
				if (!writeJsonObject(path, root))
				{
					getLogger().logError("Failed to write " + path.toStdString());
					success = false;
				}
			}
		}

		// --- CMakeSettings.json ---
		if (!snapshot.settingsExtras.isEmpty())
		{
			const QString path = projectDirPath + "/CMakeSettings.json";
			QJsonObject root;
			if (!readJsonObject(path, root))
			{
				getLogger().logWarning("Skipping settings macro re-injection: cannot read " + path.toStdString());
			}
			else
			{
				QJsonArray configs = root.value("configurations").toArray();
				for (int i = 0; i < configs.size(); ++i)
				{
					QJsonObject cfg = configs[i].toObject();
					const QString name = cfg.value("name").toString();
					if (!snapshot.settingsExtras.contains(name))
						continue;
					const ConfigExtras& extras = snapshot.settingsExtras.value(name);

					// cmakeCommandArgs
					if (!extras.extraDFlags.isEmpty())
					{
						QString args = cfg.value("cmakeCommandArgs").toString();
						const QStringList currentTokens = tokenizeCommandArgs(args);
						QSet<QString> currentKeys;
						for (const QString& t : currentTokens)
						{
							const QString k = extractDefineKey(t);
							if (!k.isEmpty())
								currentKeys.insert(k);
						}
						for (const QString& token : extras.extraDFlags)
						{
							const QString k = extractDefineKey(token);
							if (k.isEmpty() || currentKeys.contains(k))
								continue; // template wins on collision after substitution
							if (!args.isEmpty())
								args.append(' ');
							args.append(token);
							currentKeys.insert(k);
							getLogger().logInfo("Preserved user -D flag '" + token.toStdString()
								+ "' in configuration '" + name.toStdString() + "'");
						}
						cfg.insert("cmakeCommandArgs", args);
					}

					// variables[]
					if (!extras.extraVariables.isEmpty())
					{
						QJsonArray vars = cfg.value("variables").toArray();
						QSet<QString> currentVarNames;
						for (const QJsonValue& v : vars)
							currentVarNames.insert(v.toObject().value("name").toString());
						for (const QJsonValue& v : extras.extraVariables)
						{
							const QJsonObject vo = v.toObject();
							const QString vname = vo.value("name").toString();
							if (vname.isEmpty() || currentVarNames.contains(vname))
								continue;
							vars.append(vo);
							currentVarNames.insert(vname);
							getLogger().logInfo("Preserved user variable '" + vname.toStdString()
								+ "' in configuration '" + name.toStdString() + "'");
						}
						cfg.insert("variables", vars);
					}

					configs[i] = cfg;
				}
				root.insert("configurations", configs);
				if (!writeJsonObject(path, root))
				{
					getLogger().logError("Failed to write " + path.toStdString());
					success = false;
				}
			}
		}

		return success;
	}

}