#pragma once

#include "CmakeLibraryCreator_base.h"
#include "QTModule.h"
#include "Dependency.h"
#include <QVector>

namespace CLC
{
	class Resources
	{
		Resources();
		Resources(const Resources&) = delete;

		static Resources& instance();
	public:
		static QVector<QTModule> getQTModules();
		static QVector<Dependency> getDependencies();

		static void setTemplateSourcePath(const QString& path);
		static const QString &getTemplateSourcePath();

		static void setLibrarySourcePath(const QString& path);
		static const QString &getLibrarySourcePath();

		static void setTmpPath(const QString& path);
		static const QString &getTmpPath();

		static void setTemplateGitRepo(const QString& repo);
		static const QString &getTemplateGitRepo();
	private:
		QString m_templateSourcePath;
		QString m_librarySourcePath;
		QString m_tmpPath;
		
		QString m_templateGitRepo;

	};
}