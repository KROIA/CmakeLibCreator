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
	
	private:
		bool exportNewProject_intern(const ProjectSettings& settings, 
									 const QString &templateSourceDir,
								     const QString& libraryRootDir);
	
		bool copyTemplateFiles(const QString& templateSourceDir, 
							   const QString& libraryDir);
	
		bool replaceTemplateVariables(const ProjectSettings& settings, 
									  const QString& libraryDir);

		bool replaceTemplateVariablesIn_mainCmakeLists(const ProjectSettings& settings, 
													   const QString& libraryDir);
	};
}