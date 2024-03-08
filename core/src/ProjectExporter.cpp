#include "ProjectExporter.h"
#include <QDir>
#include "Utilities.h"
#include <QMessageBox>

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

	bool ProjectExporter::exportNewProject(
		const ProjectSettings& settings, 
		const QString& templateSourceDir,
		const QString& libraryRootDir)
	{
		return instance().exportNewProject_intern(settings, templateSourceDir, libraryRootDir);
	}

	bool ProjectExporter::exportNewProject_intern(
		const ProjectSettings& settings, 
		const QString& templateSourceDir,
		const QString& libraryRootDir)
	{
		bool success = true;
		templateSourceDir;
		libraryRootDir;
		const ProjectSettings::LibrarySettings &librarySettings = settings.getLibrarySettings();
		const ProjectSettings::CMAKE_settings &cmakeSettings = settings.getCMAKE_settings();
		QString projectDirPath = libraryRootDir + "/" + cmakeSettings.libraryName;

		librarySettings;
		success &= copyTemplateFiles(templateSourceDir, projectDirPath);
		// Create the project directory
		QDir dir;
		QDir projectDir(projectDirPath);
		if (!dir.exists(projectDirPath))
		{
			if (!dir.mkpath(projectDirPath))
			{
				QMessageBox::critical(0, "Error", "Failed to create project directory:\n"+ projectDirPath);
				return false;
			}
		}
		/*
		// Copy the template files to the project directory
		QDir templateDir(templateSourceDir);
		if (!templateDir.exists())
		{
			return false;
		}

		if (!copyDir(templateDir, projectDir))
		{
			return false;
		}

		// Replace the template strings in the project files
		if (!replaceTemplateStrings(settings, projectDir))
		{
			return false;
		}*/

		return true;
	}

	bool ProjectExporter::copyTemplateFiles(const QString& templateSourceDir,
											const QString& libraryDir)
	{
		// Create the project directory
		bool success = true;
		QDir dir;
		QDir projectDir(libraryDir);
		if (dir.exists(libraryDir))
		{
			if(!QMessageBox::question(0, "Folder already exists", "The folder already exists. Do you want to overwrite it?"))
				return false;
			success &= Utilities::deleteFolderRecursively(libraryDir);
		}
		
		
		if (!dir.mkpath(libraryDir))
		{
			QMessageBox::critical(0, "Error", "Failed to create project directory:\n" + libraryDir);
			return false;
		}
		
		if(success) success &= Utilities::copyAndReplaceFolderContents(templateSourceDir, libraryDir);

		return success;
	}

	bool ProjectExporter::replaceTemplateVariables(const ProjectSettings& settings,
												   const QString& libraryDir)
	{
		bool success = true;
		libraryDir;
		settings;

		

		return success;
	}

	bool ProjectExporter::replaceTemplateVariablesIn_mainCmakeLists(
		const ProjectSettings& settings,
		const QString& libraryDir)
	{
		QString cmakeListsPath = libraryDir + "/CMakeLists.txt";
		QVector<QString> fileContent = Utilities::getFileContents(cmakeListsPath);
		if (fileContent.size() == 0)
		{
			QMessageBox::critical(0, "Error", "Failed to read file:\n"+ cmakeListsPath);
			return false;
		}

		
	}

}