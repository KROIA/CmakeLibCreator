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
		static bool copyFile(const QString& from, const QString& to, bool overrideMode = false);
		static bool createFolder(const QString& folder);
		static bool deleteFile(const QString& file);
		static bool deleteFolderRecursively(const QString& folder);

		static QVector<QString> getFilesInFolder(const QString& folder, const QString& filter = "");
		static QVector<QString> getFilesInFolderRecursive(const QString& folder, const QString& filter = "");
		static QVector<QString> getFoldersInFolder(const QString& folder, const QString& filter = "");
		static QVector<QString> getFileContents(const QString& file);
		static bool saveFileContents(const QString& file, const QVector<QString>& contents);
		static bool replaceInLine(QString &line, const QString& fromPattern, const QString& toPattern, const QString &replacement);
		static bool replaceAll(QVector<QString>& lines, const QString& target, const QString& replacement);
		static bool replaceAllIfLineContains(QVector<QString>& lines, const QString& target, const QString& replacement, const QString &mustContainInLine);
	
		static int getLineIndex(const QVector<QString>& lines, const QString& pattern, bool onlyCompleteWord);
		static int getLineIndex(const QVector<QString>& lines, const QVector<QString>& patterns, bool onlyCompleteWord);
		static int getLineIndex(const QVector<QString>& lines, const QString& pattern, int startIndex, bool onlyCompleteWord);
		static int getLineIndex(const QVector<QString>& lines, const QVector<QString>& pattern, int startIndex, bool onlyCompleteWord);
		
		static bool replaceCmakeVariable(QVector<QString> &lines, QString variable, const QString& value);
		static bool replaceCmakeVariable(QVector<QString> &lines, QString variable, const QVector<QString>& values);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, QString& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, bool& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, int& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, unsigned int& value);
		static bool readCmakeVariables(const QVector<QString>& lines, QString variable, QVector<QString>& values);

		static bool replaceHeaderVariable(QVector<QString>& lines, const QString& variable, const QString& value);
		static bool readHeaderVariable(QVector<QString>& lines, const QString& variable, QString& value);
		static bool readHeaderVariable(QVector<QString>& lines, const QString& variable, int& value);
		static bool getAllIncludes(const QVector<QString>& lines, QVector<QString>& includes);
	};
}