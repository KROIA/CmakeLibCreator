#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <iostream>
#include "CmakeLibraryCreator.h"
#include "ProjectExporter.h"
#include "ProjectSettings.h"
#include "Resources.h"

#include <QWidget>
#include <QFile>


static int runCli(QApplication& app, const QStringList& args)
{
    using namespace CLC;
    Q_UNUSED(app)

    QCommandLineParser parser;
    parser.setApplicationDescription("CmakeLibCreator - CMake library template manager");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption updateOption(
        QStringList() << "u" << "update",
        "Update an existing library project with the latest template.\n"
        "Preserves all USER_SECTION blocks. Re-applies cmake files only.",
        "projectPath");
    parser.addOption(updateOption);

    QCommandLineOption templatePathOption(
        QStringList() << "t" << "template",
        "Override the template source path (default: <AppData>/data/template/main).",
        "templatePath");
    parser.addOption(templatePathOption);

    parser.process(args);

    if (!parser.isSet(updateOption))
    {
        std::cerr << "No action specified. Use --update <projectPath> or --help.\n";
        return 1;
    }

    // Initialise the Resources singleton (loads settings.json / template paths)
    Resources::loadSettings();
    Resources::loadDependencies();
    Resources::loadQTModules();

    if (parser.isSet(templatePathOption))
        Resources::setRelativeTemplateSourcePath(parser.value(templatePathOption));

    QString projectPath = QDir::cleanPath(parser.value(updateOption));
    if (!QDir(projectPath).exists())
    {
        std::cerr << "Project path does not exist: " << projectPath.toStdString() << "\n";
        return 1;
    }

    std::cout << "Reading project data from: " << projectPath.toStdString() << "\n";
    CLC::ProjectSettings settings;
    if (!CLC::ProjectExporter::readProjectData(settings, projectPath))
    {
        std::cerr << "Failed to read project data.\n";
        return 1;
    }

    CLC::ProjectExporter::ExportSettings expSettings;
    expSettings.copyAllTemplateFiles       = false;   // update only — do not wipe the project
    expSettings.replaceTemplateCmakeFiles  = true;    // update cmake/ + CMakeLists.txt files
    expSettings.replaceTemplateCodeFiles   = false;   // keep existing .h/.cpp
    expSettings.replaceTemplateVariables   = true;    // re-inject LIBRARY_NAME etc.
    expSettings.replaceTemplateCodePlaceholders = false;

    std::cout << "Applying template to: " << projectPath.toStdString() << "\n";
    if (!CLC::ProjectExporter::exportProject(settings, projectPath, expSettings))
    {
        std::cerr << "Template update failed.\n";
        return 1;
    }

    std::cout << "Done. Template applied successfully.\n";
    return 0;
}


int main(int argc, char* argv[])
{
    // Set org/app name before any QStandardPaths::AppDataLocation lookup so
    // Resources can anchor settings.json + data/ under a stable AppData root.
    QCoreApplication::setOrganizationName("KROIA");
    QCoreApplication::setApplicationName("CmakeLibCreator");

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    // If any recognised CLI arguments are present, run headlessly.
    QStringList args = app.arguments();
    bool hasCli = false;
    for (const QString& a : args)
    {
        if (a == "-u" || a == "--update" || a == "-h" || a == "--help" || a == "-v" || a == "--version")
        {
            hasCli = true;
            break;
        }
    }

    if (hasCli)
        return runCli(app, args);

    // --- GUI mode ---
    CLC::MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
