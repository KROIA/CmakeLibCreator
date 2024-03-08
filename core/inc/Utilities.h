#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>

namespace CLC
{
	class Utilities
	{
		Utilities() = delete;
		Utilities(const Utilities&) = delete;
	public:

		static bool copyAndReplaceFolderContents(const QString& absolutFromDir, const QString& absolutToDir, bool copyAndRemove = false);
		static bool deleteFolderRecursively(const QString& folder);

		static QVector<QString> getFilesInFolder(const QString& folder, const QString& filter = "*.*");
		static QVector<QString> getFoldersInFolder(const QString& folder, const QString& filter = "*.*");
		static QVector<QString> getFileContents(const QString& file);
		static bool saveFileContents(const QString& file, const QVector<QString>& contents);
		static bool replaceInLine(QString &line, const QString& fromPattern, const QString& toPattern, const QString &replacement);
	
		static int getLineIndex(const QVector<QString>& lines, const QString& pattern);
		
		static bool replaceCmakeVariable(QVector<QString> &lines, const QString& variable, const QString& value);
		static bool replaceCmakeVariable(QVector<QString> &lines, const QString& variable, const QVector<QString>& values);
	};
}