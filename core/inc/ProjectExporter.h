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
		struct ExportSettings
		{
			bool copyTemplateFiles = true;
			bool replaceTemplateFiles = true;
			bool replaceTemplateVariables = true;
			bool replaceTemplateCodePlaceholders = true;
		};
		
		

		static bool exportProject(const ProjectSettings& settings, 
									 const QString & projectDirPath,
									 const ExportSettings &expSettings);

		static bool readProjectData(ProjectSettings& settings, const QString& projectDirPath);
	
	private:
		bool exportProject_intern(const ProjectSettings& settings,
								     const QString& projectDirPath,
									 const ExportSettings& expSettings);

		bool readProjectData_intern(ProjectSettings& settings, const QString& projectDirPath);
	
		bool copyTemplateLibraryFiles(const ProjectSettings& settings,
							   const QString& projectDirPath);
		bool copyTemplateSourceFiles(const ProjectSettings& settings,
								const QString& projectDirPath);
		bool copyTemplateDependencies(const ProjectSettings& settings,
									  const QString& projectDirPath);

	
		bool replaceTemplateVariables(const ProjectSettings& settings, 
									  const QString& projectDirPath);

		bool replaceTemplateFileNames(const ProjectSettings& settings,
									  const QString& projectDirPath);


		bool replaceTemplateVariablesIn_mainCmakeLists(const ProjectSettings& settings, 
													   const QString& projectDirPath);
		bool replaceTemplateVariablesIn_cmakeSettings(const ProjectSettings& settings,
													  const QString& projectDirPath);
		bool replaceTemplateVariablesIn_libraryInfo(const ProjectSettings& settings,
													const QString& projectDirPath);

		bool replaceTemplateCodePlaceholders(const ProjectSettings& settings,
										     const QString& projectDirPath);

		bool replaceTemplateUserSectionsIn_cmakeLists(const ProjectSettings& settings,
													  const QString& projectDirPath);

		//bool saveConfiguration(const ProjectSettings& settings,
		//					   const QString& projectDirPath);


		bool readCmakeLists(ProjectSettings& settings, const QString& projectDirPath);
		bool readLibraryInfo(ProjectSettings& settings, const QString& projectDirPath);
		bool readDependencies(ProjectSettings& settings, const QString& projectDirPath);

		
		

	};
}