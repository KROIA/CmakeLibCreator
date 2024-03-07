#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QVector>

namespace CLC 
{
	class ProjectSettings
	{
	public:
		struct LibrarySettings
		{
			struct Version
			{
				int major;
				int minor;
				int patch;
				Version();
			};
			Version version;
			QString author;
			QString email;
			QString website;
			QString license;
			LibrarySettings();
		};
		struct CMAKE_settings
		{
			QString libraryName;
			QString lib_define;
			QString lib_profile_define;

			bool qt_enable;
			bool qt_deploy;

			QVector<QString> qModules;

			QString debugPostFix;
			QString staticPostFix;
			QString profilingPostFix;

			unsigned int cxxStandard;
			bool cxxStandardRequired;

			bool compile_examples;
			bool compile_unittests;

			QVector<QString> dependencies;
			CMAKE_settings();
			void autosetLibDefine();
			void autosetLibProfileDefine();
		};

		ProjectSettings();
		~ProjectSettings();

		void setLibrarySettings(const LibrarySettings& settings);
		void setCMAKE_settings(const CMAKE_settings& settings);
		const LibrarySettings& getLibrarySettings() const;
		const CMAKE_settings& getCMAKE_settings() const;

	private:


		LibrarySettings m_librarySettings;
		CMAKE_settings m_CMAKE_settings;
	};
} // namespace CLC