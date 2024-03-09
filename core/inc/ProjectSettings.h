#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QVector>
#include "QTModule.h"
#include "Dependency.h"

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

			QString namespaceName;
			QString exportName;
			LibrarySettings();
		};
		struct CMAKE_settings
		{
			QString libraryName;
			QString lib_define;
			QString lib_profile_define;
			QString lib_short_define;

			bool qt_enable;
			bool qt_deploy;

			QVector<QTModule> qModules;

			QString debugPostFix;
			QString staticPostFix;
			QString profilingPostFix;

			unsigned int cxxStandard;
			bool cxxStandardRequired;

			bool compile_examples;
			bool compile_unittests;

			QVector<Dependency> dependencies;
			CMAKE_settings();
			void autosetLibDefine();
			void autosetLibProfileDefine();
			void autosetLibShortDefine();
		};

		// Placeholder for strings that are used in the CMakeLists.txt and code files
		struct Placeholder
		{
			QString LibraryNamespace;
			QString LIBRARY_NAME_EXPORT;
			QString LibraryName;
			QString LIBRARY_NAME_LIB;
			QString LIBRARY_NAME_SHORT;


		};

		ProjectSettings();
		~ProjectSettings();

		ProjectSettings& operator=(const ProjectSettings& other);

		void setLibrarySettings(const LibrarySettings& settings);
		void setCMAKE_settings(const CMAKE_settings& settings);
		void setPlaceholder(const Placeholder& placeholder);
		const LibrarySettings& getLibrarySettings() const;
		const CMAKE_settings& getCMAKE_settings() const;
		const Placeholder& getPlaceholder() const;
		void autosetLibDefine();
		void autosetLibProfileDefine();
		void autosetLibShortDefine();

		static const Placeholder s_defaultPlaceholder;
	private:


		LibrarySettings m_librarySettings;
		CMAKE_settings m_CMAKE_settings;
		Placeholder m_placeholder;
	};
} // namespace CLC