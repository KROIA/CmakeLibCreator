#pragma once

#include "UnitTest.h"

#include "ProjectExporter.h"
#include "ProjectSettings.h"
#include "Resources.h"
#include "Utilities.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace
{
	const QString s_projectDir = QDir::tempPath() + "/CmakeLibCreatorTest_ExportRename";
	const QString s_oldName = "Foo_Driver";
	const QString s_newName = "Bar_Driver";

	// Free of the old library name on purpose, so the "no Foo_Driver survives" scans are not
	// fooled by the seed itself.
	const QString s_markerToken = "CLC_USER_SECTION_MARKER_8f21c4";
	const QString s_markerLine = "\t// " + s_markerToken;

	// Everything the exporter can rewrite; the icons are binary noise.
	bool isTextFile(const QString& path)
	{
		static const QStringList suffixes{ "h", "cpp", "c", "inl", "in", "txt", "json",
										   "cmake", "bat", "md", "qrc", "rc", "gitignore" };
		return suffixes.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive);
	}

	// Source tree only. build/ and installation/ are compile output — they legitimately keep
	// whatever name they were built under until the next "build.bat clean", and the exporter has no
	// business renaming inside them.
	QVector<QString> projectEntries()
	{
		QVector<QString> entries;
		QDirIterator it(s_projectDir, QDir::AllEntries | QDir::NoDotAndDotDot,
			QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			const QString path = QDir::fromNativeSeparators(it.next());
			if (path.contains("/build/") || path.contains("/installation/"))
				continue;
			entries.push_back(path);
		}
		return entries;
	}

	bool fileContains(const QString& filePath, const QString& needle)
	{
		for (const QString& line : CLC::Utilities::getFileContents(filePath))
		{
			if (line.contains(needle))
				return true;
		}
		return false;
	}

	// One line inside the first user section, the way a developer would add it.
	bool seedMarker(const QString& filePath)
	{
		QVector<QString> lines = CLC::Utilities::getFileContents(filePath);
		for (int i = 0; i < lines.size(); ++i)
		{
			if (!lines[i].contains("USER_SECTION_START"))
				continue;
			lines.insert(i + 1, s_markerLine);
			return CLC::Utilities::saveFileContents(filePath, lines);
		}
		return false;
	}

	// The settings dialog re-derives every dependent name whenever the library name changes.
	void applyLibraryName(CLC::ProjectSettings& settings, const QString& name)
	{
		CLC::ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		cmakeSettings.libraryName = name;
		settings.setCMAKE_settings(cmakeSettings);
		settings.autosetLibDefine();
		settings.autosetLibShortDefine();
		settings.autosetLibProfileDefine();
		settings.autoSetNamespaceName();
		settings.autoSetApiName();
	}

	// A fresh settings object deliberately preselects no Qt modules, the way the dialog presents it.
	// The generated CMakeLists.txt cannot configure with an empty QT_MODULES, so pick some — every
	// other Qt field already comes correct from the constructor.
	void applyQtModules(CLC::ProjectSettings& settings)
	{
		CLC::ProjectSettings::CMAKE_settings cmakeSettings = settings.getCMAKE_settings();
		cmakeSettings.qModules = { CLC::QTModule("Core", ""), CLC::QTModule("Widgets", "") };
		settings.setCMAKE_settings(cmakeSettings);
	}

	// Read straight out of the file rather than through the reader that is under test.
	QString cmakeVariable(const QString& filePath, const QString& variable)
	{
		const QString key = "set(" + variable;
		for (const QString& line : CLC::Utilities::getFileContents(filePath))
		{
			const QString trimmed = line.trimmed();
			if (!trimmed.startsWith(key))
				continue;
			const int close = trimmed.indexOf(')');
			if (close == -1)
				return QString();
			return trimmed.mid(key.size(), close - key.size()).trimmed();
		}
		return QString();
	}

	// A cold build downloads and builds the generated project's own dependencies.
	const int s_buildTimeoutMs = 15 * 60 * 1000;
	// Explicitly relative: cmd does not resolve a bare batch name from the working directory.
	const QString s_buildCommand = ".\\build.bat x64-Debug";

	// Not Utilities::executeCommandCapture: its waitForFinished is hard-capped at two minutes and a
	// cold build blows straight through that, leaving the child running and the output truncated.
	int runCommand(const QString& command, const QString& workingDir, QString& output, int timeoutMs)
	{
		QProcess process;
		process.setProgram("cmd");
		process.setArguments({ "/c", command });
		process.setWorkingDirectory(workingDir);
		process.setProcessChannelMode(QProcess::MergedChannels);
		process.start();
		if (!process.waitForStarted(10000))
			return -1;
		const bool finished = process.waitForFinished(timeoutMs);
		output = QString::fromLocal8Bit(process.readAll());
		if (!finished)
		{
			process.kill();
			process.waitForFinished(5000);
			return -2;
		}
		return process.exitCode();
	}

	// The lines that say why a build failed, not the thousands that say it was fine.
	QString buildFailureExcerpt(const QString& output)
	{
		const QStringList lines = output.split('\n');
		QStringList decisive;
		for (const QString& line : lines)
		{
			if (line.contains("error", Qt::CaseInsensitive) || line.contains("FAILED"))
				decisive.push_back(line.trimmed());
		}
		if (decisive.isEmpty())
			decisive = lines.mid(qMax(0, lines.size() - 25));
		return decisive.mid(0, 25).join("\n");
	}

	qint64 directorySizeMb(const QString& dir)
	{
		qint64 bytes = 0;
		QDirIterator it(dir, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
		while (it.hasNext())
		{
			it.next();
			bytes += it.fileInfo().size();
		}
		return bytes / (1024 * 1024);
	}

	// TEST_FAIL returns out of the test function, so cleanup written at the end is skipped on
	// exactly the runs that leave a half-built library tree behind. A destructor is not.
	struct TempTreeGuard
	{
		~TempTreeGuard() { QDir(s_projectDir).removeRecursively(); }
	};

	QJsonArray configurePresets(const QString& filePath)
	{
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
			return QJsonArray();
		const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
		return document.object().value("configurePresets").toArray();
	}
}

// Renaming a library on re-export used to leave the old <OldName>_* files behind and write the
// user's USER_SECTION content back into those orphans, shipping the new files empty.
class TST_exportRename : public UnitTest::Test
{
	TEST_CLASS(TST_exportRename)
public:
	TST_exportRename()
		: Test("TST_exportRename")
	{
		ADD_TEST(TST_exportRename::renameKeepsUserSectionsAndDropsTheOldName);
		// Every assertion below is independently informative, and the cleanup at the end has to run.
		setBreakOnFail(false);
	}

private:

	TEST_FUNCTION(renameKeepsUserSectionsAndDropsTheOldName)
	{
		TEST_START;

		CLC::Resources::loadSettings();
		const QString templatePath = CLC::Resources::getCurrentTemplateAbsSourcePath();
		if (!QDir(templatePath).exists())
		{
			TEST_MESSAGE("Template not available at " + templatePath.toStdString()
				+ " - the rename was NOT exercised.");
			return;
		}
		TEST_MESSAGE("Using template: " + templatePath.toStdString());

		// Start clean too, so a previous hard crash cannot poison this run.
		QDir(s_projectDir).removeRecursively();
		TempTreeGuard guard;

		CLC::ProjectSettings settings;
		applyLibraryName(settings, s_oldName);
		applyQtModules(settings);

		CLC::ProjectExporter::ExportSettings creation; // defaults copy the whole template
		TEST_ASSERT_M(CLC::ProjectExporter::exportProject(settings, s_projectDir, creation),
			"the initial export failed, so the rename could not be tested");

		const QString seededFile = s_projectDir + "/core/src/" + s_oldName + "_debug.cpp";
		TEST_ASSERT_M(seedMarker(seededFile),
			"could not insert a user section marker into " + seededFile.toStdString());
		TEST_ASSERT_M(fileContains(seededFile, s_markerToken),
			"the marker never reached the disk, the rest of this test would prove nothing");

		// Compile-verify the generated project on both sides of the rename. The pre-rename build is
		// what leaves a configured CMake cache and build output carrying the OLD file names, which is
		// the situation the post-rename build has to survive.
		//
		// THE POST-RENAME BUILD MUST RECONFIGURE, NOT JUST RECOMPILE. The generated
		// core/CMakeLists.txt collects sources with GLOB_FILES(*.h) / GLOB_FILES(*.cpp), and CMake
		// evaluates a glob at CONFIGURE time and caches the result. After the rename that cached list
		// still names Foo_Driver_*.{h,cpp} — files that no longer exist. A bare "cmake --build"
		// against the stale cache either fails on missing inputs or silently builds the wrong file
		// set, and either way the result would describe the cache rather than the rename. build.bat
		// runs "cmake --preset" first, which always configures. Do not "optimise" this into an
		// incremental rebuild.
		const bool compileRequested = !QCoreApplication::arguments().contains("--no-compile");
		bool toolchainAvailable = compileRequested;
		QString buildOutput;
		if (compileRequested)
		{
			QElapsedTimer timer;
			timer.start();
			// build.bat exits 2 for every missing-tool case and 1 for a real build failure.
			const int code = runCommand(s_buildCommand, s_projectDir, buildOutput, s_buildTimeoutMs);
			TEST_MESSAGE("Pre-rename build: " + std::to_string(timer.elapsed() / 1000) + " s, exit " + std::to_string(code));
			if (code == 2)
			{
				TEST_MESSAGE("No cmake/ninja/VS toolchain - the generated project was NOT compiled.");
				toolchainAvailable = false;
			}
			else
				TEST_ASSERT_M(code == 0, "the generated " + s_oldName.toStdString() + " project does not build:\n"
					+ buildFailureExcerpt(buildOutput).toStdString());
		}

		CLC::ProjectSettings renamed;
		TEST_ASSERT_M(CLC::ProjectExporter::readProjectData(renamed, s_projectDir),
			"reading the exported project back failed");
		applyLibraryName(renamed, s_newName);

		CLC::ProjectExporter::ExportSettings upgrade;
		upgrade.copyAllTemplateFiles = false;
		TEST_ASSERT_M(CLC::ProjectExporter::exportProject(renamed, s_projectDir, upgrade),
			"the rename export failed");

		// The regression itself.
		const QString renamedFile = s_projectDir + "/core/src/" + s_newName + "_debug.cpp";
		TEST_ASSERT_M(QFile::exists(renamedFile),
			"the renamed file " + renamedFile.toStdString() + " was never created");
		TEST_ASSERT_M(fileContains(renamedFile, s_markerToken),
			"the user's USER_SECTION content was LOST by the rename: " + s_markerToken.toStdString()
			+ " is not in " + renamedFile.toStdString());

		if (toolchainAvailable)
		{
			QElapsedTimer timer;
			timer.start();
			const int code = runCommand(s_buildCommand, s_projectDir, buildOutput, s_buildTimeoutMs);
			TEST_MESSAGE("Post-rename build: " + std::to_string(timer.elapsed() / 1000) + " s, exit " + std::to_string(code));
			TEST_MESSAGE("Temp tree peak size: " + std::to_string(directorySizeMb(s_projectDir)) + " MB");
			TEST_ASSERT_M(code == 0, "the renamed " + s_newName.toStdString() + " project does not build:\n"
				+ buildFailureExcerpt(buildOutput).toStdString());
			// Evidence the configure step really re-globbed: core/CMakeLists.txt prints every source
			// it picked up, so that listing has to name the new generation and not the old one.
			TEST_ASSERT_M(buildOutput.contains(s_newName + "_debug.cpp"),
				"the configure step did not pick up the renamed sources - the build ran off a stale glob cache");
			TEST_ASSERT_M(!buildOutput.contains(s_oldName + "_debug.cpp"),
				"the configure step still lists the previous generation's sources");
		}

		QStringList orphans;
		for (const QString& path : projectEntries())
		{
			if (QFileInfo(path).fileName().contains(s_oldName))
				orphans.push_back(path);
		}
		TEST_ASSERT_M(orphans.isEmpty(),
			"entries still named after the old library survived the rename:\n"
			+ orphans.join("\n").toStdString());

		QStringList staleLines;
		for (const QString& path : projectEntries())
		{
			const QFileInfo info(path);
			if (!info.isFile() || !isTextFile(path))
				continue;
			bool insideUserSection = false;
			int lineNr = 0;
			for (const QString& line : CLC::Utilities::getFileContents(path))
			{
				++lineNr;
				if (line.contains("USER_SECTION_START"))
					insideUserSection = true;
				else if (line.contains("USER_SECTION_END"))
					insideUserSection = false;
				else if (!insideUserSection && line.contains(s_oldName))
					staleLines.push_back(path + ":" + QString::number(lineNr) + ": " + line.trimmed());
			}
		}
		TEST_ASSERT_M(staleLines.isEmpty(),
			"the old library name survived outside every USER_SECTION:\n"
			+ staleLines.join("\n").toStdString());

		// LIB_PROFILE_DEFINE, CMakePresets.json and the debug header have to agree, or profiling
		// is enabled by a macro nothing checks.
		const QString profileDefine = cmakeVariable(s_projectDir + "/CMakeLists.txt", "LIB_PROFILE_DEFINE");
		TEST_ASSERT_M(!profileDefine.isEmpty(), "LIB_PROFILE_DEFINE is missing from CMakeLists.txt");
		TEST_COMPARE(profileDefine, renamed.getCMAKE_settings().lib_profile_define);

		const QString debugHeader = s_projectDir + "/core/inc/" + s_newName + "_debug.h";
		TEST_ASSERT_M(fileContains(debugHeader, "#ifdef " + profileDefine),
			debugHeader.toStdString() + " does not guard on " + profileDefine.toStdString());

		int profilePresets = 0;
		QStringList presetProblems;
		for (const QJsonValue& value : configurePresets(s_projectDir + "/CMakePresets.json"))
		{
			const QJsonObject preset = value.toObject();
			const QString presetName = preset.value("name").toString();
			if (!presetName.endsWith("-Profile"))
				continue;
			++profilePresets;

			QStringList profilingKeys;
			const QJsonObject cacheVariables = preset.value("cacheVariables").toObject();
			for (const QString& key : cacheVariables.keys())
			{
				if (key.endsWith("_PROFILING"))
					profilingKeys.push_back(key);
			}

			if (profilingKeys.size() != 1)
				presetProblems.push_back(presetName + " carries " + QString::number(profilingKeys.size())
					+ " profiling cache variables: " + profilingKeys.join(", "));
			else if (profilingKeys.first() != profileDefine)
				presetProblems.push_back(presetName + " enables " + profilingKeys.first()
					+ " while CMakeLists.txt defines " + profileDefine);
		}
		TEST_ASSERT_M(profilePresets > 0, "CMakePresets.json has no -Profile preset to check");
		TEST_ASSERT_M(presetProblems.isEmpty(),
			"profiling is silently disabled:\n" + presetProblems.join("\n").toStdString());
	}
};

TEST_INSTANTIATE(TST_exportRename);
