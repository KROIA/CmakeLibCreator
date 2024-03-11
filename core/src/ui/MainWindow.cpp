#include "ui/MainWindow.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDialog>
#include <QDirIterator>
#include "Resources.h"
#include <QDebug>
#include "Utilities.h"
#include "ProjectExporter.h"
#include "CmakeLibraryCreator_info.h"

namespace CLC
{
	MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent)
	{
		ui.setupUi(this);
		setWindowTitle("CMake Library Creator");
		//qApp->setStyleSheet(Resources::getStyleSheet());


		m_ribbon = new CLC::RibbonImpl(ui.ribbon_widget);
		m_projectSettingsDialog = new ProjectSettingsDialog(this);
		ui.centralWidget->layout()->addWidget(m_projectSettingsDialog);

		RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
		RibbonImpl::ProjectButtons projectButtons = RibbonImpl::getProjectButtons();
		connect(templateManagementButtons.openTemplatePath, &QPushButton::clicked, this, &MainWindow::onOpenTemplatePath_clicked);
		connect(templateManagementButtons.downloadTemplate, &QPushButton::clicked, this, &MainWindow::onDownloadTemplate_clicked);
		connect(projectButtons.openExistingProject, &QPushButton::clicked, this, &MainWindow::onOpenExistingProject_clicked);
		connect(projectButtons.saveExistingProject, &QPushButton::clicked, this, &MainWindow::onSaveExistingProject_clicked);
		connect(projectButtons.saveAsNewProject, &QPushButton::clicked, this, &MainWindow::onSaveAsNewProject_clicked);

		m_settingsDialog = new SettingsDialog();
	}

	MainWindow::~MainWindow()
	{
		delete m_ribbon;
		delete m_projectSettingsDialog;
		delete m_settingsDialog;
	}


	void MainWindow::onOpenTemplatePath_clicked()
	{
		// Open file dialog to select a folder
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Template Path"),
			Resources::getTemplateSourcePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if(folderPath.size() == 0)
		{
			return;
		}
		QDir dir;
		if(!dir.exists(folderPath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The selected folder does not exist", QMessageBox::Ok, this);
			return;
		}
		Resources::setTemplateSourcePath(folderPath);
	}
	void MainWindow::onDownloadTemplate_clicked()
	{
		// Download a git repository
		const Resources::GitResources &gitResources = Resources::getTemplateGitRepo();
		QString gitRepoUrl = gitResources.repo;
		QString gitRepoBranch = gitResources.templateBranch;
		QString tmpPath = Resources::getTmpPath()+ "/git";
		Utilities::downloadGitRepository(gitRepoUrl, gitRepoBranch, Resources::getTemplateSourcePath(), tmpPath);
		Utilities::downloadGitRepository(gitRepoUrl, "dependencies", tmpPath);
		Utilities::downloadGitRepository(gitRepoUrl, "qtModules", tmpPath);

		// Copy the dependencies and qtModules to the template source path
		QString workingDir = QDir::currentPath();
		Utilities::copyAndReplaceFolderContents(workingDir + "/"+ tmpPath + "/dependencies/dependencies", workingDir + "/" + Resources::getDependenciesSourcePath());
		Utilities::copyAndReplaceFolderContents(workingDir + "/"+ tmpPath + "/qtModules/qtModules", workingDir + "/" + Resources::getQtModulesSourcePath());
		
		QDir tmpDir1(workingDir + "/" + tmpPath + "/dependencies");
		tmpDir1.removeRecursively();
		QDir tmpDir2(workingDir + "/" + tmpPath + "/qtModules");
		tmpDir2.removeRecursively();
	
		Resources::loadQTModules();
		Resources::loadDependencies();
	}
	
	void MainWindow::onOpenExistingProject_clicked()
	{
		// Open file dialog to select a folder
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Library Path"),
			Resources::getLoadedProjectPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (folderPath.size() == 0)
		{
			return;
		}
		QDir dir;
		if (!dir.exists(folderPath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The selected folder does not exist", QMessageBox::Ok, this);
			return;
		}
		Resources::setLoadedProjectPath(folderPath);
		ProjectSettings settings;
		ProjectExporter::readProjectData(settings, folderPath);
		m_projectSettingsDialog->setSettings(settings);
		m_existingProjectLoaded = true;
	}
	void MainWindow::onSaveExistingProject_clicked()
	{
		if (!m_existingProjectLoaded)
		{
			QMessageBox box(QMessageBox::Warning, "Error", "No project loaded", QMessageBox::Ok, this);
			return;
		}
		QString templateSourcePath = Resources::getCurrentTemplateAbsSourcePath();
		QDir dir;
		if(!dir.exists(templateSourcePath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The template source path does not exist,\ndownload the template first", QMessageBox::Ok, this);
			return;
		}


		ProjectExporter::ExportSettings exportSettings;
		exportSettings.copyTemplateFiles = false;
		exportSettings.replaceTemplateFiles = true;
		exportSettings.replaceTemplateVariables = true;
		exportSettings.replaceTemplateCodePlaceholders = true;
		ProjectExporter::exportProject(m_projectSettingsDialog->getSettings(), Resources::getLoadedProjectPath(), exportSettings);

		ProjectSettings settings;
		ProjectExporter::readProjectData(settings, Resources::getLoadedProjectPath());
		m_projectSettingsDialog->setSettings(settings);

	}

	void MainWindow::onSaveAsNewProject_clicked()
	{
		// Open file dialog to select a folder
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Libraries root path"),
			Resources::getLoadedProjectPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (folderPath.size() == 0)
		{
			return;
		}
		QDir dir;
		if (!dir.exists(folderPath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The selected folder does not exist", QMessageBox::Ok, this);
			return;
		}
		
		QString templateSourcePath = Resources::getCurrentTemplateAbsSourcePath();
		if (!dir.exists(templateSourcePath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The template source path does not exist,\ndownload the template first", QMessageBox::Ok, this);
			return;
		}
		
		auto settings = m_projectSettingsDialog->getSettings();
		settings.setPlaceholder(ProjectSettings::s_defaultPlaceholder);
		folderPath += "/" + settings.getCMAKE_settings().libraryName;

		ProjectExporter::ExportSettings exportSettings;
		exportSettings.copyTemplateFiles = true;
		exportSettings.replaceTemplateFiles = true;
		exportSettings.replaceTemplateVariables = true;
		exportSettings.replaceTemplateCodePlaceholders = true;

		Resources::setLoadedProjectPath(folderPath);
		ProjectExporter::exportProject(settings, folderPath, exportSettings);
		
		ProjectSettings settings2;
		ProjectExporter::readProjectData(settings2, folderPath+"/"+ settings.getCMAKE_settings().libraryName);
		m_projectSettingsDialog->setSettings(settings2);
		m_existingProjectLoaded = true;
	}
	void MainWindow::on_actionVersion_triggered()
	{
		// Display UI with version information		
		QWidget *w = LibraryInfo::createInfoWidget();
		w->setWindowTitle("Version");
		// auto delete on close
		w->setAttribute(Qt::WA_DeleteOnClose);
		w->show();
	}
	void MainWindow::on_actionAbout_triggered()
	{
		// Display UI with version information		
		QWidget *w = LibraryInfo::createInfoWidget();
		w->setWindowTitle("About");
		// Add label with description 
		QHBoxLayout *layout = new QHBoxLayout();
		QLabel *label = new QLabel("This application is a tool to create CMake based C++ libraries for QT.");
		layout->addWidget(label);
		w->layout()->addItem(layout);
		// auto delete on close
		w->setAttribute(Qt::WA_DeleteOnClose);
		w->show();
	}
	void MainWindow::on_actionSettings_triggered()
	{
		m_settingsDialog->show();
	}



	
}
