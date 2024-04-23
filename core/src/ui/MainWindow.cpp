#include "ui/MainWindow.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDialog>
#include <QDirIterator>
#include <QThread>
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
		m_projectSettingsDialog = new ProjectSettingsDialog();
		ui.centralWidget->layout()->addWidget(m_projectSettingsDialog);

		RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
		RibbonImpl::ProjectButtons projectButtons = RibbonImpl::getProjectButtons();
		connect(templateManagementButtons.openTemplatePath, &QPushButton::clicked, this, &MainWindow::onOpenTemplatePath_clicked);
		connect(templateManagementButtons.downloadTemplate, &QPushButton::clicked, this, &MainWindow::onDownloadTemplate_clicked);
		connect(projectButtons.openExistingProject, &QPushButton::clicked, this, &MainWindow::onOpenExistingProject_clicked);
		connect(projectButtons.saveExistingProject, &QPushButton::clicked, this, &MainWindow::onSaveExistingProject_clicked);
		connect(projectButtons.saveAsNewProject, &QPushButton::clicked, this, &MainWindow::onSaveAsNewProject_clicked);
		connect(projectButtons.loadAndSaveAll, &QPushButton::clicked, [this]() {
			loadAndSaveProjects(Resources::getLoadSaveProjects().projectPaths);
			});

		m_settingsDialog = new SettingsDialog();

		m_exportSettingsDialog = new CheckBoxSelectionDialog("Export Settings");
		QVector<CheckBoxSelectionDialog::Element> elements = {
			//{"Copy template files", false},
			{"Replace template CMake files", true, "Replaces the cmake files from the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template code files", true, "Replaces the src files from the core folder of the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template variables", true, "Replaces placeholder variables in cmake files, such as:\n\"LIBRARY_NAME\", \"QT_MODULES\", ..."},
			{"Replace template code placeholders", true, "Replaces placeholder variables in source code files, such as:\n\"CmakeLibraryCreator\", \"CMAKE_LIBRARY_CREATOR_EXPORT\", \"CMAKELIBRARYCREATOR_LIB\", ..."}
		};
		m_exportSettingsDialog->setItems(elements);
		m_exportSettingsDialog->hide();
		connect(m_exportSettingsDialog, &CheckBoxSelectionDialog::okButtonClicked, this, &MainWindow::onExportDialogOkButtonClicked);
		connect(m_exportSettingsDialog, &CheckBoxSelectionDialog::dialogClosed, this, [this]() {
				RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
				if (prjButtons.saveExistingProject)
					prjButtons.saveExistingProject->enableLoadingCircle(false);
				if (prjButtons.saveAsNewProject)
					prjButtons.saveAsNewProject->enableLoadingCircle(false);
			});

		connect(&Utilities::instance(), &Utilities::signalInformation, this, &MainWindow::signalInformation, Qt::QueuedConnection);
		connect(&Utilities::instance(), &Utilities::signalWarning, this, &MainWindow::signalWarning, Qt::QueuedConnection);
		connect(&Utilities::instance(), &Utilities::signalCritical, this, &MainWindow::signalCritical, Qt::QueuedConnection);


		//m_timer.setInterval(100);
		//connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
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
			Resources::getRelativeTemplateSourcePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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
		// Remove the current dir from the folderPath
		QString currentDir = QDir::currentPath();
		if (folderPath.startsWith(currentDir))
		{
			folderPath = folderPath.right(folderPath.size() - currentDir.size() - 1);
		}
		Resources::setRelativeTemplateSourcePath(folderPath);
	}
	void MainWindow::onDownloadTemplate_clicked()
	{
		
		// Download a git repository
		m_workerThread = new QThread;
		RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
		if(templateManagementButtons.downloadTemplate)
			templateManagementButtons.downloadTemplate->enableLoadingCircle(true);

		// worker lambda
		auto worker = [this]()
		{
			
			const Resources::GitResources& gitResources = Resources::getTemplateGitRepo();
			QString gitRepoUrl = gitResources.repo;
			QString gitRepoBranch = gitResources.templateBranch;
			QString tmpPath = Resources::getRelativeTmpPath() + "/git";
			Utilities::downloadGitRepository(gitRepoUrl, gitRepoBranch, Resources::getRelativeTemplateSourcePath(), tmpPath);
			Utilities::downloadGitRepository(gitRepoUrl, "dependencies", tmpPath);
			Utilities::downloadGitRepository(gitRepoUrl, "qtModules", tmpPath);

			// Copy the dependencies and qtModules to the template source path
			QString workingDir = QDir::currentPath();
			Utilities::copyAndReplaceFolderContents(workingDir + "/" + tmpPath + "/dependencies/dependencies", workingDir + "/" + Resources::getRelativeDependenciesSourcePath());
			Utilities::copyAndReplaceFolderContents(workingDir + "/" + tmpPath + "/qtModules/qtModules", workingDir + "/" + Resources::getRelativeQtModulesSourcePath());

			QDir tmpDir1(workingDir + "/" + tmpPath + "/dependencies");
			tmpDir1.removeRecursively();
			QDir tmpDir2(workingDir + "/" + tmpPath + "/qtModules");
			tmpDir2.removeRecursively();

			Resources::loadQTModules();
			Resources::loadDependencies();
			this->m_workerThread->exit();
		};
		// move worker to thread
		QObject::connect(m_workerThread, &QThread::started, worker);
		QObject::connect(m_workerThread, &QThread::finished, this, [this]() {
			threadFinished();
			});
		//QObject::connect(m_workerThread, &QThread::, this, &MainWindow::enableUI);
		disableUI();
		//m_timer.start();
		m_workerThread->start();

	}
	
	void MainWindow::onOpenExistingProject_clicked()
	{
		
		

		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Library Path"),
			Resources::getLoadedProjectPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (folderPath.size() == 0)
		{
			return;
		}
		if (!loadProjectAsync(folderPath))
			return;
		RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
		if (prjButtons.openExistingProject)
			prjButtons.openExistingProject->enableLoadingCircle(true);
		disableUI();
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
		if (!dir.exists(templateSourcePath))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The template source path does not exist,\ndownload the template first", QMessageBox::Ok, this);
			return;
		}
		
		m_exportSettingsDialog->show();
		//this->m_workerThread->exit();
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



		QString _folderPath = folderPath;
		auto settings = m_projectSettingsDialog->getSettings();
		settings.setLoadedPlaceholder(ProjectSettings::s_defaultPlaceholder);
		_folderPath += "/" + settings.getCMAKE_settings().libraryName;
		Resources::setLoadedProjectPath(_folderPath);
		if (dir.exists(_folderPath))
		{
			if (!QMessageBox::question(0, "Folder already exists", "The folder already exists. Do you want to overwrite it?"))
				return;
		}
		

		m_workerThread = new QThread;

		static ProjectSettings loadingSettings;
		loadingSettings = ProjectSettings();
			// worker lambda
		auto worker = [this, folderPath, settings, _folderPath]()
			{
				ProjectExporter::ExportSettings exportSettings;
				exportSettings.copyAllTemplateFiles = true;
				exportSettings.replaceTemplateCmakeFiles = true;
				exportSettings.replaceTemplateCodeFiles = true;
				exportSettings.replaceTemplateVariables = true;
				exportSettings.replaceTemplateCodePlaceholders = true;

				
				ProjectExporter::exportProject(settings, _folderPath, exportSettings);

				//ProjectSettings settings2;
				ProjectExporter::readProjectData(loadingSettings, _folderPath);
				
				this->m_workerThread->exit();
			};
		// move worker to thread
		QObject::connect(m_workerThread, &QThread::started, worker);
		QObject::connect(m_workerThread, &QThread::finished, this, [this]() {
			m_projectSettingsDialog->setSettings(loadingSettings);
			m_existingProjectLoaded = true;
			threadFinished();
			});

		RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
		disableUI();
		if (prjButtons.saveAsNewProject)
			prjButtons.saveAsNewProject->enableLoadingCircle(true);
		m_workerThread->start();
	}
	void MainWindow::onExportDialogOkButtonClicked(const QVector<CheckBoxSelectionDialog::Element>& selectedItems)
	{
		m_workerThread = new QThread;
		/*RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
		if (prjButtons.openExistingProject)
			prjButtons.openExistingProject->enableLoadingCircle(true);*/

		static ProjectSettings loadingSettings;
		loadingSettings = ProjectSettings();
		// worker lambda
		auto worker = [this, selectedItems]()
			{
				ProjectExporter::ExportSettings exportSettings;
				exportSettings.copyAllTemplateFiles = false;
				exportSettings.replaceTemplateCmakeFiles = selectedItems[0].selected;
				exportSettings.replaceTemplateCodeFiles = selectedItems[1].selected;
				exportSettings.replaceTemplateVariables = selectedItems[2].selected;
				exportSettings.replaceTemplateCodePlaceholders = selectedItems[3].selected;
				ProjectExporter::exportProject(m_projectSettingsDialog->getSettings(), Resources::getLoadedProjectPath(), exportSettings);

				
				ProjectExporter::readProjectData(loadingSettings, Resources::getLoadedProjectPath());
				
				this->m_workerThread->exit();
			};
		// move worker to thread
		QObject::connect(m_workerThread, &QThread::started, worker);
		QObject::connect(m_workerThread, &QThread::finished, this, [this]() {
			m_projectSettingsDialog->setSettings(loadingSettings);
			threadFinished();
			});
		//QObject::connect(m_workerThread, &QThread::, this, &MainWindow::enableUI);
		RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
		disableUI();
		if (prjButtons.saveExistingProject)
			prjButtons.saveExistingProject->enableLoadingCircle(true);
		//m_timer.start();
		m_workerThread->start();
	}

	void MainWindow::disableUI()
	{
		ui.centralWidget->setEnabled(false);
		update();
	}
	void MainWindow::enableUI()
	{
		ui.centralWidget->setEnabled(true);
		update();
	}

	void MainWindow::signalInformation(const QString& title,
		const QString& text)
	{
		QMessageBox::information(nullptr, title, text, QMessageBox::Button::Ok, QMessageBox::Button::Ok);
	}
	void MainWindow::signalWarning(const QString& title,
		const QString& text)
	{
		QMessageBox::warning(nullptr, title, text, QMessageBox::Button::Ok, QMessageBox::Button::Ok);
	}
	void MainWindow::signalCritical(const QString& title,
		const QString& text)
	{
		QMessageBox::critical(nullptr, title, text, QMessageBox::Button::Ok, QMessageBox::Button::Ok);
	}

	/*void MainWindow::onTimerTimeout()
	{
		if (m_workerThread)
		{
			if (m_workerThread->isFinished())
			{
				enableUI();
				m_timer.stop();
				delete m_workerThread;
				m_workerThread = nullptr;

				RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
				if (templateManagementButtons.downloadTemplate)
					templateManagementButtons.downloadTemplate->enableLoadingCircle(false);

				RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
				if (prjButtons.openExistingProject)
					prjButtons.openExistingProject->enableLoadingCircle(false);
				if (prjButtons.saveExistingProject)
					prjButtons.saveExistingProject->enableLoadingCircle(false);
				if (prjButtons.saveAsNewProject)
					prjButtons.saveAsNewProject->enableLoadingCircle(false);
			}
		}
	}*/
	void MainWindow::threadFinished()
	{
		if (m_workerThread)
		{
			if (m_workerThread->isFinished())
			{
				enableUI();
			//	m_timer.stop();
				delete m_workerThread;
				m_workerThread = nullptr;

				RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
				if (templateManagementButtons.downloadTemplate)
					templateManagementButtons.downloadTemplate->enableLoadingCircle(false);

				RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
				if (prjButtons.openExistingProject)
					prjButtons.openExistingProject->enableLoadingCircle(false);
				if (prjButtons.saveExistingProject)
					prjButtons.saveExistingProject->enableLoadingCircle(false);
				if (prjButtons.saveAsNewProject)
					prjButtons.saveAsNewProject->enableLoadingCircle(false);
			}
		}
	}

	bool MainWindow::loadProjectAsync(const QString& path)
	{
		QDir dir;
		if (!dir.exists(path))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The selected folder does not exist", QMessageBox::Ok, this);
			return false;
		}
		// Open file dialog to select a folder
		m_workerThread = new QThread;
		static ProjectSettings loadingSettings;
		loadingSettings = ProjectSettings();

		// worker lambda
		auto worker = [this, path]()
			{
				Resources::setLoadedProjectPath(path);
				//ProjectSettings settings;
				ProjectExporter::readProjectData(loadingSettings, path);
				this->m_workerThread->exit();
			};

		// move worker to thread
		QObject::connect(m_workerThread, &QThread::started, worker);
		QObject::connect(m_workerThread, &QThread::finished, this, [this]() {
			m_projectSettingsDialog->setSettings(loadingSettings);
			m_existingProjectLoaded = true;
			threadFinished();
			});

		m_workerThread->start();
		return true;
	}
	void MainWindow::loadAndSaveProjects(QStringList paths)
	{
		if (paths.size() == 0)
			return;

		QString path = paths[0];
		paths.erase(paths.begin());
		QDir dir;
		if (!dir.exists(path))
		{
			QMessageBox box(QMessageBox::Warning, "Error", "The selected folder does not exist", QMessageBox::Ok, this);
			return;
		}
		// Open file dialog to select a folder
		m_workerThread = new QThread;
		static ProjectSettings loadingSettings;
		loadingSettings = ProjectSettings();

		// worker lambda
		auto worker = [this, path]()
			{
				Resources::setLoadedProjectPath(path);
				//ProjectSettings settings;
				ProjectExporter::readProjectData(loadingSettings, path);
				this->m_workerThread->exit();
			};

		// move worker to thread
		QObject::connect(m_workerThread, &QThread::started, worker);
		QObject::connect(m_workerThread, &QThread::finished, this, [this, paths]() {
			m_projectSettingsDialog->setSettings(loadingSettings);
			m_existingProjectLoaded = true;
			if (m_workerThread)
			{
				if (m_workerThread->isFinished())
				{
					//enableUI();
					//	m_timer.stop();
					delete m_workerThread;
					m_workerThread = nullptr;

					RibbonImpl::TemplateManagementButtons templateManagementButtons = RibbonImpl::getTemplateManagementButtons();
					if (templateManagementButtons.downloadTemplate)
						templateManagementButtons.downloadTemplate->enableLoadingCircle(false);

					RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
					if (prjButtons.openExistingProject)
						prjButtons.openExistingProject->enableLoadingCircle(false);
					if (prjButtons.saveExistingProject)
						prjButtons.saveExistingProject->enableLoadingCircle(false);
					if (prjButtons.saveAsNewProject)
						prjButtons.saveAsNewProject->enableLoadingCircle(false);


					m_workerThread = new QThread;
					/*RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
					if (prjButtons.openExistingProject)
						prjButtons.openExistingProject->enableLoadingCircle(true);*/

					//static ProjectSettings loadingSettings;
					//loadingSettings = ProjectSettings();
					// worker lambda
					auto worker = [this]()
						{
							ProjectExporter::ExportSettings exportSettings;
							exportSettings.copyAllTemplateFiles = false;
							exportSettings.replaceTemplateCmakeFiles = true;
							exportSettings.replaceTemplateCodeFiles = true;
							exportSettings.replaceTemplateVariables = true;
							exportSettings.replaceTemplateCodePlaceholders = true;
							ProjectExporter::exportProject(m_projectSettingsDialog->getSettings(), Resources::getLoadedProjectPath(), exportSettings);


							//ProjectExporter::readProjectData(loadingSettings, Resources::getLoadedProjectPath());

							this->m_workerThread->exit();
						};
					// move worker to thread
					QObject::connect(m_workerThread, &QThread::started, worker);
					QObject::connect(m_workerThread, &QThread::finished, this, [this, paths]() {
							if (paths.size() == 0)
							{

							}
							else
								loadAndSaveProjects(paths);
						});
					//QObject::connect(m_workerThread, &QThread::, this, &MainWindow::enableUI);
					//RibbonImpl::ProjectButtons prjButtons = RibbonImpl::getProjectButtons();
					//disableUI();
					//if (prjButtons.saveExistingProject)
					//	prjButtons.saveExistingProject->enableLoadingCircle(true);
					//m_timer.start();
					m_workerThread->start();
				}

				
			}


			});

		m_workerThread->start();
	}
	
	void MainWindow::on_actionVersion_triggered()
	{
		// Display UI with version information		
		QWidget *w = LibraryInfo::createInfoWidget(this);
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
