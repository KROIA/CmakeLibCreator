#include "ui/MainWindow.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDirIterator>
#include "Resources.h"
#include <QDebug>
#include "Utilities.h"
#include "ProjectExporter.h"

namespace CLC
{
	MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent)
	{
		ui.setupUi(this);
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

	}

	MainWindow::~MainWindow()
	{
		delete m_ribbon;
		delete m_projectSettingsDialog;
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
		QString gitRepoUrl = Resources::getTemplateGitRepo();
		QString folderPath = Resources::getTemplateSourcePath();
		QString tmpPath = Resources::getTmpPath()+ "/git";
		QDir tmpDir(tmpPath);
		if(tmpDir.exists())
		{
			tmpDir.removeRecursively();
		}
		QDir dir;
		dir.mkpath(tmpPath);

		QString gitCommand = "git clone " + gitRepoUrl + " " + tmpPath;
		qDebug() << gitCommand;
		system(gitCommand.toStdString().c_str());
		
		// Check if tmpPath contains files
		
		QStringList files = tmpDir.entryList(QDir::Files);
		if(files.size() == 0)
		{
			QMessageBox box(QMessageBox::Warning, "Error", "No files found in the git repository", QMessageBox::Ok, this);
			return;
		}

		QDir folderDir(folderPath);
		// Remove all files in folderPath
		if(folderDir.exists())
		{
			QStringListIterator it(folderDir.entryList(QDir::Files));
			while(it.hasNext())
			{
				QString file = it.next();
				folderDir.remove(file);
			}
		}
		else
		{
			dir.mkpath(folderPath);
		}
		// copy all files and folders from tmpPath to folderPath
		QString currentDir = QDir::currentPath();
		Utilities::copyAndReplaceFolderContents(currentDir+"/"+tmpPath, currentDir + "/" + folderPath, true);
		// Remove tmpPath
		tmpDir.removeRecursively();
	}
	
	void MainWindow::onOpenExistingProject_clicked()
	{
		// Open file dialog to select a folder
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Library Path"),
			Resources::getDependenciesSourcePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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
		Resources::setDependenciesSourcePath(folderPath);
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
		QString templateSourcePath = QDir::currentPath() + "/" + Resources::getTemplateSourcePath();
		ProjectExporter::exportExistingProject(m_projectSettingsDialog->getSettings(), templateSourcePath, Resources::getDependenciesSourcePath());

		ProjectSettings settings;
		ProjectExporter::readProjectData(settings, Resources::getDependenciesSourcePath());
		m_projectSettingsDialog->setSettings(settings);

	}

	void MainWindow::onSaveAsNewProject_clicked()
	{
		// Open file dialog to select a folder
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Libraries root path"),
			Resources::getTemplateSourcePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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
		QString templateSourcePath = QDir::currentPath() + "/" + Resources::getTemplateSourcePath();
		Resources::setDependenciesSourcePath(folderPath);
		auto settings = m_projectSettingsDialog->getSettings();
		settings.setPlaceholder(ProjectSettings::s_defaultPlaceholder);
		ProjectExporter::exportNewProject(settings, templateSourcePath, folderPath);
		
		ProjectSettings settings2;
		ProjectExporter::readProjectData(settings2, folderPath+"/"+ settings.getCMAKE_settings().libraryName);
		m_projectSettingsDialog->setSettings(settings2);
		m_existingProjectLoaded = true;
	}



	
}
