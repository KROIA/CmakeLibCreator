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
	bool Utilities::copyFile(const QString& from, const QString& to, bool overrideMode)
	{
		QFile file(to);
		if (file.exists())
		{
			if(overrideMode)
				file.remove();
			else
				return false;
		}
		// create path if it does not exist
		QDir dir;
		dir.mkpath(QFileInfo(to).absolutePath());

		return QFile::copy(from, to);
	}
	bool Utilities::createFolder(const QString& folder)
	{
		QDir dir;
		return dir.mkpath(folder);
	}
	bool Utilities::deleteFile(const QString& file)
	{
		QFile f(file);
		return f.remove();
	}
	bool Utilities::deleteFolderRecursively(const QString& folder)
	{
		QDir dir(folder);
		return dir.removeRecursively();
	}

	QVector<QString> Utilities::getFilesInFolder(const QString& folder, const QString& filter)
	{
		QVector<QString> files;
		QDirIterator it(folder, QDir::Files, QDirIterator::NoIteratorFlags);
		while (it.hasNext()) {
			it.next();
			const auto fileInfo = it.fileInfo();
			if (!fileInfo.isHidden()) { //filters dot and dotdot
				if (filter == "*.*" || filter == "")
				{
					files.push_back(fileInfo.absoluteFilePath());
				}
				else
					if (fileInfo.fileName().contains(filter))
						files.push_back(fileInfo.absoluteFilePath());
			}
		}
		return files;
	}
	QVector<QString> Utilities::getFilesInFolderRecursive(const QString& folder, const QString& filter)
	{
		QVector<QString> files;
		QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			const auto fileInfo = it.fileInfo();
			if (!fileInfo.isHidden()) { //filters dot and dotdot
				if (filter == "*.*" || filter == "")
				{
					files.push_back(fileInfo.absoluteFilePath());
				}
				else
					if (fileInfo.fileName().contains(filter))
						files.push_back(fileInfo.absoluteFilePath());
			}
		}
		return files;
	}
	QVector<QString> Utilities::getFoldersInFolder(const QString& folder, const QString& filter)
	{
		QVector<QString> folders;

		QDir directory(folder);

		// Check if the provided path is a valid directory
		if (!directory.exists()) {
			qWarning("Directory does not exist.");
			return folders;
		}

		// Set filter to include only directories and skip hidden directories
		directory.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

		// Apply custom filter if provided
		if (!filter.isEmpty())
			directory.setNameFilters(QStringList(filter));

		// Get list of directories
		QStringList folderList = directory.entryList();

		// Append directory names to the vector
		for (const QString& folderName : folderList) {
			QString folderPath = directory.absoluteFilePath(folderName);
			folders.push_back(folderPath);
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
		for (int i = 0; i < contents.size(); ++i)
		{
			out << contents[i] << "\n";
		}
		f.close();
		return true;
	}
	bool Utilities::replaceInLine(QString& line, const QString& fromPattern, const QString& toPattern, const QString& replacement)
	{
		int index1 = line.indexOf(fromPattern);
		
		if (index1 == -1)
			return false;
		QString left = line.mid(0, index1);
		QString right = line.mid(index1);
		int index2 = right.indexOf(toPattern);
		if(index2 == -1)
			return false;
		
		right = right.mid(index2);
		line = left + replacement + right;
		return true;
	}
	bool Utilities::replaceAll(QVector<QString>& lines, const QString& target, const QString& replacement)
	{
		bool hasChanged = false;
		for (QString& line : lines)
		{
			int pos = line.indexOf(target);
			while (pos != -1)
			{
				line.replace(target, replacement);
				hasChanged = true;
				pos = line.indexOf(target, pos + replacement.size());
			}
		}
		return hasChanged;
	}
	bool Utilities::replaceAllIfLineContains(QVector<QString>& lines, const QString& target, const QString& replacement, const QVector<QVector<QString>>& mustContainInLine)
	{
		bool hasChanged = false;
		if(target == replacement)
			return false;
		for (QString& line : lines)
		{
			bool doReplacement = false;
			if(mustContainInLine.size() == 0)
				doReplacement = true;
			for (const QVector<QString>& andList : mustContainInLine)
			{
				bool containsAll = true;
				for (const QString& mustContain : andList)
				{
					if (!line.contains(mustContain))
					{
						containsAll = false;
						break;
					}
				}
				doReplacement |= containsAll;
			}
			if (!doReplacement)
				continue;

			int pos = line.indexOf(target);
			while (pos != -1)
			{
				line.replace(target, replacement);
				hasChanged = true;
				pos = line.indexOf(target, pos + replacement.size());
			}
		}
		return hasChanged;
	}

	int Utilities::getLineIndex(const QVector<QString>& lines, const QString& pattern, bool onlyCompleteWord)
	{
		return getLineIndex(lines, pattern, 0, onlyCompleteWord);
	}
	int Utilities::getLineIndex(const QVector<QString>& lines, const QVector<QString>& patterns, bool onlyCompleteWord)
	{
		return getLineIndex(lines, patterns, 0, onlyCompleteWord);
	}
	int Utilities::getLineIndex(const QVector<QString>& lines, const QString& pattern, int startIndex, bool onlyCompleteWord)
	{
		for (int i = startIndex; i < lines.size(); i++)
		{
			if (lines[i].contains(pattern))
			{
				if (onlyCompleteWord)
				{
					const QString& line = lines[i];
					int index = line.indexOf(pattern);
					QChar before = (index == 0 ? ' ' : line[index - 1]);
					QChar after = (index + pattern.size() >= line.size() ? ' ' : line[index + pattern.size()]);
					if (before.isLetterOrNumber() || after.isLetterOrNumber())
						continue;
					else
						return i;
				}
				else
					return i;
			}
		}
		return -1;
	}
	int Utilities::getLineIndex(const QVector<QString>& lines, const QVector<QString>& patterns, int startIndex, bool onlyCompleteWord)
	{
		for (int i = startIndex; i < lines.size(); i++)
		{
			bool foundAll = true;
			for (const auto& pattern : patterns)
			{
				if (lines[i].contains(pattern))
				{
					if (onlyCompleteWord)
					{
						const QString& line = lines[i];
						int index = line.indexOf(pattern);
						QChar before = (index == 0 ? ' ' : line[index - 1]);
						QChar after = (index + pattern.size() >= line.size() ? ' ' : line[index + pattern.size()]);
						if (before.isLetterOrNumber() || after.isLetterOrNumber())
							foundAll = false;
					}
				}
				else
				{
					foundAll = false;
				}

			}
			if (foundAll)
				return i;
		}
		return -1;
	}

	bool Utilities::replaceCmakeVariable(QVector<QString>& lines, QString variable, const QString& value)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString line = lines[lineIndex];
		QString left = line.mid(0, line.indexOf(variable));
		line = line.mid(line.indexOf(variable) + variable.length());
		QString right = line.mid(line.indexOf(")")+1);
		QString replacement = left + variable +" "+ value + ")" + right;
		lines[lineIndex] = replacement;
		return true;
	}
	bool Utilities::replaceCmakeVariable(QVector<QString>& lines, QString variable, const QVector<QString>& values)
	{
		int startLineIndex = getLineIndex(lines, variable, true);
		if (startLineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		int endLineIndex = getLineIndex(lines, ")", startLineIndex);
		if (endLineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find end of variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString left = lines[startLineIndex].mid(0, lines[startLineIndex].indexOf(variable));
		//int lineCount = endLineIndex - startLineIndex;
		lines.erase(lines.begin() + startLineIndex, lines.begin() + endLineIndex + 1);
		QVector<QString> replacement;
		
		for (const auto& value : values)
			replacement.push_back("  " + value);
		if(replacement.size())
			replacement[0] = left + variable + "\n" + replacement[0];
		else
			replacement.push_back(left + variable);
		replacement[replacement.size()-1] += ")";
		for(int i=replacement.size()-1; i>=0; i--)
			lines.insert(lines.begin() + startLineIndex, replacement[i]);

		return true;
	}
	bool Utilities::replaceUserCodeSections(const QString& filePath, const QVector<UserSection>& sections)
	{
		return replaceUserSections(filePath, sections, "//");
	}
	bool Utilities::replaceUserCodeSections(QVector<QString>& lines, const QVector<UserSection>& sections)
	{
		return replaceUserSections(lines, sections, "//");
	}
	bool Utilities::replaceUserCmakeSections(const QString& filePath, const QVector<UserSection>& sections)
	{
		return replaceUserSections(filePath, sections, "#");
	}
	bool Utilities::replaceUserCmakeSections(QVector<QString>& lines, const QVector<UserSection>& sections)
	{
		return replaceUserSections(lines, sections, "#");
	}
	bool Utilities::replaceUserSections(const QString& filePath, const QVector<UserSection>& sections, const QString& commentSign)
	{
		QVector<QString> lines = getFileContents(filePath);
		bool success = replaceUserSections(lines, sections, commentSign);
		if (success)
			saveFileContents(filePath, lines);
		return success;
	}
	bool Utilities::replaceUserSections(QVector<QString>& lines, const QVector<UserSection>& sections, const QString& commentSign)
	{
		QVector<QString> newLines;
		bool success = true;
		for (int i = 0; i < lines.size(); i++)
		{
			const QString startPattern1 = "USER_SECTION_START";
			const QString startPattern2 = commentSign;
			int patternIndex1 = lines[i].indexOf(startPattern1);
			int patternIndex2 = lines[i].indexOf(startPattern2);
			if (patternIndex1 >= 0 && patternIndex2 >= 0 && patternIndex1 > patternIndex2 && success)
			{
				int sectionIndex = -1;
				bool ok = false;
				sectionIndex = lines[i].mid(patternIndex1 + startPattern1.size()).trimmed().toInt(&ok);
				if (!ok)
				{
					QMessageBox::critical(nullptr, "Error", "Invalid user section index in CMakeLists.txt Line: " + QString::number(i) + " : " + lines[i]);
					sectionIndex = -1;
				}
				int sectionListIndex = -1;
				for (int j=0; j<sections.size(); ++j)
				{
					if (sections[j].sectionIndex == sectionIndex)
					{
						sectionListIndex = j;
						break;
					}
				}
				if (sectionListIndex == -1)
				{
					newLines.push_back(lines[i]);
					continue;
				}
				else
				{

					for (const auto& line : sections[sectionListIndex].lines)
						newLines.push_back(line);
				}
				
				int endIndex = getLineIndex(lines, "USER_SECTION_END", i, false);
				if (endIndex == -1)
				{
					QMessageBox::critical(nullptr, "Error", "Could not find end of user section in CMakeLists.txt");
					success = false;
				}
				else
					i = endIndex;
				continue;
			}
			newLines.push_back(lines[i]);
		}
		lines = newLines;
		return success;
	}

	bool Utilities::readCmakeVariable(const QVector<QString>& lines, QString variable, QString& value)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf(variable) + variable.length());
		value = line.mid(0, line.indexOf(")")).trimmed();
		return true;
	}
	bool Utilities::readCmakeVariable(const QVector<QString>& lines, QString variable, bool& value)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf(variable) + variable.length());
		QString valueStr = line.mid(0, line.indexOf(")")).toLower().trimmed();

		if(valueStr == "on" ||
		   valueStr == "true")
			value = true;
		else
			value = false;

		return true;
	}
	bool Utilities::readCmakeVariable(const QVector<QString>& lines, QString variable, int& value)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf(variable) + variable.length());
		QString valueStr = line.mid(0, line.indexOf(")")).trimmed();

		bool success = true;
		value = valueStr.toInt(&success);
		return success;
	}
	bool Utilities::readCmakeVariable(const QVector<QString>& lines, QString variable, unsigned int& value)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf(variable) + variable.length());
		QString valueStr = line.mid(0, line.indexOf(")")).trimmed();

		bool success = true;
		value = valueStr.toUInt(&success);
		return success;
	}
	bool Utilities::readCmakeVariables(const QVector<QString>& lines, QString variable, QVector<QString>& values)
	{
		int lineIndex = getLineIndex(lines, variable, true);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in CMakeLists.txt");
			return false;
		}
		int endLineIndex = getLineIndex(lines, ")", lineIndex, false);
		if (endLineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find end of variable " + variable + " in CMakeLists.txt");
			return false;
		}
		QString firstElement = lines[lineIndex];
		firstElement = firstElement.mid(firstElement.indexOf(variable) + variable.length()).trimmed();
		bool multiLine = true;
		if (firstElement.contains(")"))
		{
			firstElement = firstElement.mid(0, firstElement.indexOf(")")).trimmed();
			multiLine = false;
		}
		// split by space
		int spacePos = -1;
		while ((spacePos = firstElement.indexOf(" ")) != -1)
		{
			QString element = firstElement.mid(0, spacePos).trimmed();
			if (element.size())
				values.push_back(element.trimmed());
			firstElement = firstElement.mid(spacePos + 1);
		}
		if(firstElement.size())
			values.push_back(firstElement.trimmed());
		if (multiLine)
		{
			for (int i = lineIndex + 1; i <= endLineIndex; i++)
			{
				QString line = lines[i];
				if(line.indexOf(")") != -1)
					line = line.mid(0, line.indexOf(")")).trimmed();
				if(line.size())
					values.push_back(line.trimmed());
			}
		}

		return true;
	}

	bool Utilities::readUserCodeSections(const QString& filePath, QVector<UserSection>& sections)
	{
		return readUserSections(filePath, sections, "//");
	}
	bool Utilities::readUserCodeSections(const QVector<QString>& lines, QVector<UserSection>& sections)
	{
		return readUserSections(lines, sections, "//");
	}
	bool Utilities::readUserCmakeSections(const QString& filePath, QVector<UserSection>& sections)
	{
		return readUserSections(filePath, sections, "#");
	}
	bool Utilities::readUserCmakeSections(const QVector<QString>& lines, QVector<UserSection>& sections)
	{
		return readUserSections(lines, sections, "#");
	}

	
	bool Utilities::readUserSections(const QString& filePath, QVector<UserSection>& sections, const QString& commentSign)
	{
		QVector<QString> lines = getFileContents(filePath);
		return readUserSections(lines, sections, commentSign);
	}
	bool Utilities::readUserSections(const QVector<QString>& lines, QVector<UserSection>& sections, const QString& commentSign)
	{
		bool success = true;
		for (int i = 0; i < lines.size(); i++)
		{
			const QString startPattern1 = "USER_SECTION_START";
			const QString startPattern2 = commentSign;
			int patternIndex1 = lines[i].indexOf(startPattern1);
			int patternIndex2 = lines[i].indexOf(startPattern2);
			if (patternIndex1 >= 0 && patternIndex2 >= 0 && patternIndex1 > patternIndex2 && success)
			{
				int sectionIndex = -1;
				bool ok = false;
				UserSection section;
				sectionIndex = lines[i].mid(patternIndex1 + startPattern1.size()).trimmed().toInt(&ok);
				if (!ok)
				{
					QMessageBox::critical(nullptr, "Error", "Invalid user section index in CMakeLists.txt Line: " + QString::number(i)+" : "+lines[i]);
					section.sectionIndex = -1;
				}
				else
					section.sectionIndex = sectionIndex;
				success &= ok;

				section.lines.push_back(lines[i]);
				i++;
				while (!lines[i].contains("USER_SECTION_END"))
				{
					section.lines.push_back(lines[i]);
					i++;
				}
				section.lines.push_back(lines[i]);
				sections.push_back(section);
			}
		}
		return success;
	}

	bool Utilities::replaceHeaderVariable(QVector<QString>& lines, const QString& variable, const QString& value)
	{
		int lineIndex = getLineIndex(lines, { variable, "=", ";"}, false);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in header file");
			return false;
		}
		QString line = lines[lineIndex];
		QString left = line.mid(0,line.indexOf("="));
		QString right = line.mid(line.indexOf(";")+1);
		QString replacement = left + "= " + value + ";" + right;
		lines[lineIndex] = replacement;
		return true;
	}
	bool Utilities::readHeaderVariable(QVector<QString>& lines, const QString& variable, QString& value)
	{
		int lineIndex = getLineIndex(lines, { variable, "=", ";" }, false);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in header file");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf("=") + 1);
		value = line.mid(0, line.indexOf(";")).trimmed();
		if (value.contains("\""))
		{
			if (value.indexOf("\"") != 0)
			{
				QMessageBox::critical(nullptr, "Error", "Invalid string format for variable " + variable + " in header file. Value = "+ value);
				return false;
			}
			if (value.lastIndexOf("\"") != value.size() - 1)
			{
				QMessageBox::critical(nullptr, "Error", "Invalid string format for variable " + variable + " in header file. Value = " + value);
				return false;
			}
			value = value.mid(1, value.size() - 2);
		}
		return true;
	}
	bool Utilities::readHeaderVariable(QVector<QString>& lines, const QString& variable, int& value)
	{
		int lineIndex = getLineIndex(lines, { variable, "=", ";" }, false);
		if (lineIndex == -1)
		{
			QMessageBox::critical(nullptr, "Error", "Could not find variable " + variable + " in header file");
			return false;
		}
		QString line = lines[lineIndex];
		line = line.mid(line.indexOf("=") + 1);
		value = line.mid(0, line.indexOf(";")).trimmed().toInt();
		return true;
	}
	bool Utilities::getAllIncludes(const QVector<QString>& lines, QVector<QString>& includes)
	{
		for (const auto& line : lines)
		{
			if (line.contains("#include"))
				includes.push_back(line);
		}
		return true;
	}
	bool Utilities::downloadGitRepository(const QString& url, const QString& branch, const QString& folder, QString tmpFolder)
	{
		if(!downloadGitRepository(url, branch, tmpFolder))
			return false;

		QDir dir;
		//QDir folderDir(folder);
		deleteFolderRecursively(folder+"/"+branch);
		// Remove all files in folderPath
		/*if (folderDir.exists())
		{
			QStringListIterator it(folderDir.entryList(QDir::Files));
			while (it.hasNext())
			{
				QString file = it.next();
				folderDir.remove(file);
			}
		}
		else
		{
			dir.mkpath(folder);
		}*/
		dir.mkpath(folder + "/" + branch);
		// copy all files and folders from tmpPath to folderPath
		QString currentDir = QDir::currentPath();
		copyAndReplaceFolderContents(currentDir + "/" + tmpFolder, currentDir + "/" + folder, true);
		// Remove tmpPath
		QDir tmpDir(tmpFolder+"/"+branch);
		tmpDir.removeRecursively();
		return true;
	}
	bool Utilities::downloadGitRepository(const QString& url, const QString& branch, QString folder)
	{
		// Download a git repository
		folder = folder + "/" + branch;
		QDir tmpDir(folder);
		if (tmpDir.exists())
		{
			tmpDir.removeRecursively();
		}
		QDir dir;
		dir.mkpath(folder);

		QString gitCommand = "git clone --branch " + branch + " " + url + " " + folder;

		qDebug() << gitCommand;
		system(gitCommand.toStdString().c_str());
		QStringList files = tmpDir.entryList(QDir::Files);
		if (files.size() == 0)
		{
			QMessageBox box(QMessageBox::Warning, "Error", "No files found in the git repository", QMessageBox::Ok);
			return false;
		}
		return true;
	}
}
