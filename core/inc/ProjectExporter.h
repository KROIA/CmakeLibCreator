#pragma once

#include "CmakeLibraryCreator_base.h"
#include "ProjectSettings.h"

namespace CLC
{
	class ProjectExporter
	{
		ProjectExporter();
		static ProjectExporter& instance();
	public:
		
		

		static bool exportNewProject(const ProjectSettings& settings, 
									 const QString &templateSourceDir,
									 const QString &libraryRootDir);

		static bool exportExistingProject(const ProjectSettings& settings,
									 const QString& templateSourceDir,
									 const QString& projectDirPath);

		static bool readProjectData(ProjectSettings& settings, const QString& projectDirPath);
	
	private:
		bool exportNewProject_intern(const ProjectSettings& settings, 
									 const QString &templateSourceDir,
								     const QString& libraryRootDir);
		bool exportExistingProject_internal(const ProjectSettings& settings,
											const QString& templateSourceDir,
											const QString& projectDirPath);

		bool readProjectData_intern(ProjectSettings& settings, const QString& projectDirPath);
	
		bool copyTemplateLibraryFiles(const ProjectSettings& settings, 
							   const QString& templateSourceDir,
							   const QString& libraryDir);
		bool copyTemplateSourceFiles(const ProjectSettings& settings,
								const QString& templateSourceDir,
								const QString& libraryDir);
	
		bool replaceTemplateVariables(const ProjectSettings& settings, 
									  const QString& libraryDir);

		bool replaceTemplateFileNames(const ProjectSettings& settings,
									  const QString& libraryDir);


		bool replaceTemplateVariablesIn_mainCmakeLists(const ProjectSettings& settings, 
													   const QString& libraryDir);
		bool replaceTemplateVariablesIn_cmakeSettings(const ProjectSettings& settings,
													  const QString& libraryDir);
		bool replaceTemplateVariablesIn_libraryInfo(const ProjectSettings& settings,
													const QString& libraryDir);

		bool replaceTemplateCodePlaceholders(const ProjectSettings& settings,
										     const QString& libraryDir);

		//bool saveConfiguration(const ProjectSettings& settings,
		//					   const QString& libraryDir);


		bool readCmakeLists(ProjectSettings& settings, const QString& projectDirPath);
		bool readLibraryInfo(ProjectSettings& settings, const QString& projectDirPath);
		bool readDependencies(ProjectSettings& settings, const QString& projectDirPath);

		
		

	};
}