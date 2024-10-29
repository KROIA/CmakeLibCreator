#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QString>
#include <QStringList>
#include <QObject>
#include <QMessageBox>
#include "Logging.h"

namespace CLC
{
	class Utilities: public QObject
	{
		Q_OBJECT
		Utilities()
		{

		}
		//Utilities(const Utilities&) = delete;

		static constexpr size_t maxFileOpenRetryCount = 10;
		static constexpr size_t fileOpenRetryDelay = 50; // ms
		
	public:
		static Utilities& instance();
		static Log::LogObject& getLogger();
		struct UserSection
		{
			int sectionIndex;
			QStringList lines;
		};

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
		
		// QVector<QVector<QString>> &mustContainInLine is a list of strings that must be in the line to be replaced
		// 2D to support and , or conditions
		// Example:
		// QVector<QVector<QString>> mustContainInLine1 = {{"if", "(", "WIN32", ")"}}; AND condition of all the strings
		// QVector<QVector<QString>> mustContainInLine2 = {{"if", "(", "WIN32", ")"}, {"if", "(", "UNIX", ")"}}; OR condition between the 2 vectors
 		static bool replaceAllIfLineContains(QVector<QString>& lines, const QString& target, const QString& replacement, const QVector<QVector<QString>> &mustContainInLine);
	
		// QString commentKey means the string that defines a comment in the file
		// Example: "#" for CMake, "//" for C++, etc.
		// If it is empty, comments are not ignored
		static int getLineIndex(const QVector<QString>& lines, const QString& pattern, bool onlyCompleteWord, const QString& commentKey);
		static int getLineIndex(const QVector<QString>& lines, const QVector<QString>& patterns, bool onlyCompleteWord, const QString& commentKey);
		static int getLineIndex(const QVector<QString>& lines, const QString& pattern, int startIndex, bool onlyCompleteWord, const QString& commentKey);
		static int getLineIndex(const QVector<QString>& lines, const QVector<QString>& pattern, int startIndex, bool onlyCompleteWord, const QString& commentKey);
		
		static bool isTextInsideString(const QString& line, const QString& targetText);
		static QString getLineWithoutComments(const QString& line, const QString& commentKey);

		static bool replaceCmakeVariable(QVector<QString> &lines, QString variable, const QString& value);
		static bool replaceCmakeVariableString(QVector<QString> &lines, QString variable, const QString& value);
		static bool replaceCmakeVariable(QVector<QString> &lines, QString variable, const QVector<QString>& values);
		static bool replaceUserCodeSections(const QString& filePath, const QVector<UserSection>& sections);
		static bool replaceUserCodeSections(QVector<QString>& lines, const QVector<UserSection>& sections);
		static bool replaceUserCmakeSections(const QString& filePath, const QVector<UserSection>& sections);
		static bool replaceUserCmakeSections(QVector<QString>& lines, const QVector<UserSection>& sections);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, QString& value);
		static bool readCmakeVariableString(const QVector<QString>& lines, QString variable, QString& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, bool& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, int& value);
		static bool readCmakeVariable(const QVector<QString>& lines, QString variable, unsigned int& value);
		static bool readCmakeVariables(const QVector<QString>& lines, QString variable, QVector<QString>& values);

		static bool readUserCodeSections(const QString& filePath, QVector<UserSection>& sections);
		static bool readUserCodeSections(const QVector<QString>& lines, QVector<UserSection>& sections);
		static bool readUserCmakeSections(const QString& filePath, QVector<UserSection>& sections);
		static bool readUserCmakeSections(const QVector<QString>& lines, QVector<UserSection>& sections);

		static bool replaceHeaderVariable(QVector<QString>& lines, const QString& variable, const QString& value);
		static bool readHeaderVariable(QVector<QString>& lines, const QString& variable, QString& value);
		static bool readHeaderVariable(QVector<QString>& lines, const QString& variable, int& value);
		static bool getAllIncludes(const QVector<QString>& lines, QVector<QString>& includes);
	
		static bool downloadGitRepository(const QString& url, const QString &branch, const QString& folder, QString tmpFolder);
		static bool downloadGitRepository(const QString& url, const QString &branch, QString folder);

		static bool gitHasUncommitedChanges(const QString& folder);
		static bool gitHasUnpushedCommits(const QString& folder);
		static bool gitPull(const QString& folder);
		static bool gitCommit(const QString& folder, const QString& message);
		static bool gitPush(const QString& folder);

		static void information(const QString& title, const QString& text);
		static void warning(const QString& title, const QString& text);
		static void critical(const QString& title, const QString& text);

		static int executeCommand(const QString& command);
		static int executeCommand(const QString& command, Log::LogObject &logger);

	signals:
		void signalInformation(const QString& title, const QString& text);
		void signalWarning(const QString& title, const QString& text);
		void signalCritical(const QString& title, const QString& text);
	
	private:
		static bool readUserSections(const QString& filePath, QVector<UserSection>& sections, const QString &commentSign = "//");
		static bool readUserSections(const QVector<QString>& lines, QVector<UserSection>& sections, const QString& commentSign = "//");

		static bool replaceUserSections(const QString& filePath, const QVector<UserSection>& sections, const QString &commentSign = "//");
		static bool replaceUserSections(QVector<QString>& lines, const QVector<UserSection>& sections, const QString &commentSign = "//");
	
		static bool openFile(QFile &file, QIODevice::OpenMode flags);

		void _information(const QString& title, const QString& text);
		void _warning(const QString& title, const QString& text);
		void _critical(const QString& title, const QString& text);
	};
}
