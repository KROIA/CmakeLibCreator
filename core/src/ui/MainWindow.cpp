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

		m_exportSettingsDialog = new CheckBoxSelectionDialog("Export Settings");
		QVector<CheckBoxSelectionDialog::Element> elements = {
			//{"Copy template files", false},
			{"Replace template CMake files", false, "Replaces the cmake files from the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template code files", false, "Replaces the src files from the core folder of the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template variables", false, "Replaces placeholder variables in cmake files, such as:\n\"LIBRARY_NAME\", \"QT_MODULES\", ..."},
			{"Replace template code placeholders", false, "Replaces placeholder variables in source code files, such as:\n\"CmakeLibraryCreator\", \"CMAKE_LIBRARY_CREATOR_EXPORT\", \"CMAKELIBRARYCREATOR_LIB\", ..."}
		};
		m_exportSettingsDialog->setItems(elements);
		m_exportSettingsDialog->hide();
		connect(m_exportSettingsDialog, &CheckBoxSelectionDialog::okButtonClicked, this, &MainWindow::onExportDialogOkButtonClicked);

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
		m_exportSettingsDialog->show();
	}
	void MainWindow::onExportDialogOkButtonClicked(const QVector<CheckBoxSelectionDialog::Element>& selectedItems)
	{
		ProjectExporter::ExportSettings exportSettings;
		exportSettings.copyAllTemplateFiles = false;
		exportSettings.replaceTemplateCmakeFiles = selectedItems[0].selected;
		exportSettings.replaceTemplateCodeFiles = selectedItems[1].selected;
		exportSettings.replaceTemplateVariables = selectedItems[2].selected;
		exportSettings.replaceTemplateCodePlaceholders = selectedItems[3].selected;
		ProjectExporter::exportProject(m_projectSettingsDialog->getSettings(), Resources::getLoadedProjectPath(), exportSettings);

		ProjectSettings settings;
		ProjectExporter::readProjectData(settings, Resources::getLoadedProjectPath());
		m_projectSettingsDialog->setSettings(settings);
	}


	void MainWindow::onSaveAsNewProject_clicked()
	{
		// Open file dialog to select a folder
		QString loadedProjectPath = Resources::getLoadedProjectPath();
		loadedProjectPath = QFileInfo(loadedProjectPath).path();
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Libraries root path"),
			loadedProjectPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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
		exportSettings.copyAllTemplateFiles = true;
		exportSettings.replaceTemplateCmakeFiles = true;
		exportSettings.replaceTemplateCodeFiles = true;
		exportSettings.replaceTemplateVariables = true;
		exportSettings.replaceTemplateCodePlaceholders = true;

		Resources::setLoadedProjectPath(folderPath);
		ProjectExporter::exportProject(settings, folderPath, exportSettings);
		
		ProjectSettings settings2;
		ProjectExporter::readProjectData(settings2, folderPath);
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
