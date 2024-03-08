#include "Utilities.h"
#include <QDebug>
#include <QDirIterator>
#include <QMessageBox>

namespace CLC
{
	bool Utilities::copyAndReplaceFolderContents(const QString& absolutFromDir, const QString& absolutToDir, bool copyAndRemove)
	{
		qDebug() << "Copying from " << absolutFromDir << " to " << absolutToDir;
		QDirIterator it(absolutFromDir, QDirIterator::Subdirectories);
		QDir dir(absolutFromDir);
		const int absSourcePathLength = dir.absoluteFilePath(absolutFromDir).length();

		while (it.hasNext()) {
			it.next();
			const auto fileInfo = it.fileInfo();
			if (!fileInfo.isHidden()) { //filters dot and dotdot
				const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
				const QString constructedAbsolutePath = absolutToDir + subPathStructure;

				if (fileInfo.isDir()) {
					//Create directory in target folder
					dir.mkpath(constructedAbsolutePath);
				}
				else if (fileInfo.isFile()) {
					//Copy File to target directory

					//Remove file at target location, if it exists, or QFile::copy will fail
					QFile::remove(constructedAbsolutePath);
					QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
				}
			}
		}

		if (copyAndRemove)
			dir.removeRecursively();
		return 1;
	}
	bool Utilities::deleteFolderRecursively(const QString& folder)
	{
		QDir dir(folder);
		return dir.removeRecursively();
	}

	QVector<QString> Utilities::getFilesInFolder(const QString& folder, const QString& filter)
	{
		QVector<QString> files;
		QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			const auto fileInfo = it.fileInfo();
			if (!fileInfo.isHidden()) { //filters dot and dotdot
				if (fileInfo.fileName().contains(filter))
					files.push_back(fileInfo.absoluteFilePath());
			}
		}
		return files;
	}
	QVector<QString> Utilities::getFoldersInFolder(const QString& folder, const QString& filter)
	{
		QVector<QString> folders;
		QDirIterator it(folder, QDir::Dirs, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			const auto fileInfo = it.fileInfo();
			if (!fileInfo.isHidden()) { //filters dot and dotdot
				if (fileInfo.fileName().contains(filter))
					folders.push_back(fileInfo.absoluteFilePath());
			}
		}
		return folders;
	}
	QVector<QString> Utilities::getFileContents(const QString& file)
	{
		QVector<QString> lines;
		QFile f(file);
		if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
			return lines;

		QTextStream in(&f);
		while (!in.atEnd()) {
			lines.push_back(in.readLine());
		}
		f.close();
		return lines;
	}
	bool Utilities::saveFileContents(const QString& file, const QVector<QString>& contents)
	{
		QFile f(file);
		if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
			return false;

		QTextStream out(&f);
		for (const auto& line : contents)
			out << line << "\n";
		f.close();
		return true;
	}
	bool Utilities::replaceInLine(QString& line, const QString& fromPattern, const QString& toPattern, const QString& replacement)
	{
		int index1 = line.indexOf(fromPattern);
		
		if (index1 == -1)
			return false;
		QString left = line.left(index1);
		QString right = line.right(index1);
		int index2 = right.indexOf(toPattern);
		if(index2 == -1)
			return false;
		
		right = right.right(index2);
		line = left + replacement + right;
		return true;
	}

	int Utilities::getLineIndex(const QVector<QString>& lines, const QString& pattern)
	{
		for(int i=0; i<lines.size(); i++)
		{
			if(lines[i].contains(pattern))
				return i;
		}
		return -1;
	}

	bool Utilities::replaceCmakeVariable(QVector<QString>& lines, const QString& variable, const QString& value)
	{
		int lineIndex = getLineIndex(lines, variable);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		// Replace the cmake parameter with the pattern: set(LIB_DEFINE CMAKE_LIB_CREATOR_LIB)  
		QString left = lines[lineIndex].left(lines[lineIndex].indexOf(variable));
		QString replacement = "set(" + variable + " " + value + ")";
	}
	bool Utilities::replaceCmakeVariable(QVector<QString>& lines, const QString& variable, const QVector<QString>& values)
	{

	}

}