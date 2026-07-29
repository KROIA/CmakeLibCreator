#include "ui/MainWindow.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDialog>
#include <QDirIterator>
#include <QThread>
#include "Resources.h"
#include <QDebug>
#include <QInputDialog>
#include <QTabWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QLineEdit>
#include "Utilities.h"
#include "ProjectExporter.h"
#include "CmakeLibraryCreator_info.h"
#include "Logging.h"


namespace CLC
{
	MainWindow::MainWindow(QWidget* parent)
		: QMainWindow(parent)
	{
		ui.setupUi(this);
		
		setWindowTitle("CMake Library Creator");
		//qApp->setStyleSheet(Resources::getStyleSheet());
		QToolBar* tb = new QToolBar(this);
		tb->setMovable(false);
		tb->setFixedHeight(130);
		addToolBar(Qt::TopToolBarArea, tb);

		m_ribbon = new CLC::RibbonImpl(tb);
		m_projectSettingsDialog = new ProjectSettingsDialog();
		setupCentralTabs();

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
			{"Replace template CMake files", true, "Replaces the cmake files from the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template code files", true, "Replaces the src files from the core folder of the template with the existing ones in the library.\nCode in the user sections will be preserved."},
			{"Replace template variables", true, "Replaces placeholder variables in cmake files, such as:\n\"LIBRARY_NAME\", \"QT_MODULES\", ..."},
			{"Replace template code placeholders", true, "Replaces placeholder variables in source code files, such as:\n\"CmakeLibraryCreator\", \"CMAKE_LIBRARY_API\", \"CMAKELIBRARY_LIB\", ..."}
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

		setupStatusBar();
		setupRepositoryConnections();

		Log::Color::setDarkMode(true);
		Log::Resources::getIconDebug();
		Logging::getView().setStyleSheet(Log::Resources::getDarkStylesheet());
		Logging::getView().show();
		//m_timer.setInterval(100);
		//connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);

		// Populate page 2 at startup: local git commands only, fast.
		{
			QVector<RepositoryJobQueue::Job> jobs;
			for (const auto& e : Resources::getLoadSaveProjects().projects)
				jobs.push_back({ e.path, RepositoryJobQueue::JobType::RefreshStatus, {} });
			if (!jobs.isEmpty())
				m_jobQueue->enqueue(jobs);
		}
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
		if (m_jobQueue->isBusy()) { Logging::getLogger().logWarning("A repository operation is running"); return; }
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

			// Copy the dependencies and qtModules to the template source path.
			// tmpPath and the Resources paths are absolute (AppData-anchored).
			Utilities::copyAndReplaceFolderContents(tmpPath + "/dependencies/dependencies", Resources::getRelativeDependenciesSourcePath());
			Utilities::copyAndReplaceFolderContents(tmpPath + "/qtModules/qtModules", Resources::getRelativeQtModulesSourcePath());

			QDir tmpDir1(tmpPath + "/dependencies");
			tmpDir1.removeRecursively();
			QDir tmpDir2(tmpPath + "/qtModules");
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
		if (m_jobQueue->isBusy()) { Logging::getLogger().logWarning("A repository operation is running"); return; }

		const QString loaded = Resources::getLoadedProjectPath();
		const QString initialDir = loaded.isEmpty() ? Resources::getDefaultLibraryPath() : loaded;
		QString folderPath = QFileDialog::getExistingDirectory(this, tr("Open Library Path"),
			initialDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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
		if (m_jobQueue->isBusy()) { Logging::getLogger().logWarning("A repository operation is running"); return; }
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
		if (m_jobQueue->isBusy()) { Logging::getLogger().logWarning("A repository operation is running"); return; }
		if (m_workerThread)
		{
			if (m_workerThread->isRunning())
			{
				Logging::getLogger().logWarning("A thread is already running");
				return;
			}
		}
		// Open file dialog to select a folder
		QString loadedProjectPath = Resources::getLoadedProjectPath();
		loadedProjectPath = QFileInfo(loadedProjectPath).path();
		if (loadedProjectPath.isEmpty())
			loadedProjectPath = Resources::getDefaultLibraryPath();
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
		if (m_jobQueue->isBusy()) { Logging::getLogger().logWarning("A repository operation is running"); return; }
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
	void MainWindow::closeEvent(QCloseEvent* event)
	{
		CLC_UNUSED(event);
		// Stop the tab-sync lambdas from touching widgets that are being torn down.
		m_closing = true;
		if (m_ribbon)
			disconnect(m_ribbon->getTabWidget(), &QTabWidget::currentChanged, this, nullptr);
		if (m_centralTabs)
			disconnect(m_centralTabs, &QTabWidget::currentChanged, this, nullptr);
		if (m_jobQueue && m_jobQueue->isBusy())
			m_jobQueue->shutdown(5000);
		if (m_buildRunner)
			m_buildRunner->shutdown(5000);   // kill in-flight build trees + bounded join
		Logging::getView().close();
		// Close the application
		QApplication::quit();
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

	void MainWindow::setupCentralTabs()
	{
		m_repositoryOverview = new RepositoryOverviewWidget();
		m_centralTabs = new QTabWidget();
		m_centralTabs->addTab(m_projectSettingsDialog, "Project");
		m_centralTabs->addTab(m_repositoryOverview, "Repositories");
		ui.centralWidget->layout()->addWidget(m_centralTabs);

		// keep ribbon tab and central page in sync (indices align: 0=Project, 1=Repositories).
		// setCurrentIndex only emits currentChanged on an actual change -> the cross-connection converges.
		QTabWidget* ribbonTabs = m_ribbon->getTabWidget();
		connect(ribbonTabs, &QTabWidget::currentChanged, this, [this](int i) {
			if (m_closing) return;
			if (i >= 0 && i < m_centralTabs->count()) m_centralTabs->setCurrentIndex(i);
			});
		connect(m_centralTabs, &QTabWidget::currentChanged, this, [this, ribbonTabs](int i) {
			if (m_closing) return;
			if (i >= 0 && i < ribbonTabs->count()) ribbonTabs->setCurrentIndex(i);
			});
	}

	void MainWindow::setupStatusBar()
	{
		m_statusLabel = new QLabel();
		m_buildStatusLabel = new QLabel();
		m_statusProgress = new QProgressBar();
		m_statusProgress->setMaximumWidth(200);
		m_statusProgress->setTextVisible(true);
		m_cancelButton = new QPushButton("Cancel");
		ui.statusBar->addWidget(m_statusLabel, 1);
		ui.statusBar->addPermanentWidget(m_buildStatusLabel);
		ui.statusBar->addPermanentWidget(m_statusProgress);
		ui.statusBar->addPermanentWidget(m_cancelButton);
		m_statusProgress->hide();
		m_cancelButton->hide();
		connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
			m_cancelButton->setEnabled(false);
			m_statusLabel->setText("Canceling…");
			m_jobQueue->cancel();
			m_buildRunner->cancelAll();   // parallel builds are canceled alongside the queue
			});
	}

	void MainWindow::setupRepositoryConnections()
	{
		m_jobQueue = new RepositoryJobQueue(this);
		m_unitTestRunner = new UnitTestRunner(this);
		m_buildRunner = new BuildRunner(this);
		m_buildRunner->setMaxThreads(Resources::getMaxBuildThreads());
		m_buildResultDialog = new JobResultDialog("Build results");
		m_testResultDialog = new JobResultDialog("Unittest results");
		// result-row "Show log" reuses the shared TextLogWindow with the repo's captured log.
		connect(m_buildResultDialog, &JobResultDialog::showLogRequested, this, [this](const QString& path, const QString&) {
			if (!m_testLogWindow) m_testLogWindow = new TextLogWindow();
			m_testLogWindow->showLog("Build log — " + QDir(path).dirName(), m_repositoryOverview->infoFor(path).buildLog);
			});
		connect(m_testResultDialog, &JobResultDialog::showLogRequested, this, [this](const QString& path, const QString& subKey) {
			if (!m_testLogWindow) m_testLogWindow = new TextLogWindow();
			const RepositoryInfo info = m_repositoryOverview->infoFor(path);
			if (subKey.isEmpty())
			{
				m_testLogWindow->showLog("Unittest log — " + QDir(path).dirName(), info.unitTestLog);
				return;
			}
			for (const UnitTestSuiteResult& s : info.unitTestSuites)
			{
				if (s.suiteName == subKey)
				{
					m_testLogWindow->showLog("Unittest log — " + QDir(path).dirName() + " / " + s.suiteName, s.log);
					return;
				}
			}
			m_testLogWindow->showLog("Unittest log — " + QDir(path).dirName(), info.unitTestLog);
			});
		// queue -> overview (per-repo state)
		connect(m_jobQueue, &RepositoryJobQueue::statusRefreshed,  m_repositoryOverview, &RepositoryOverviewWidget::onStatusRefreshed);
		connect(m_jobQueue, &RepositoryJobQueue::templateUpdated,  m_repositoryOverview, &RepositoryOverviewWidget::onTemplateUpdated);
		// build runner -> overview (off-queue parallel pool, per repo)
		connect(m_buildRunner, &BuildRunner::started,  m_repositoryOverview, &RepositoryOverviewWidget::onBuildStarted);
		connect(m_buildRunner, &BuildRunner::finished, m_repositoryOverview, &RepositoryOverviewWidget::onBuildFinished);
		// unittest runner -> overview (off-queue, per repo)
		connect(m_unitTestRunner, &UnitTestRunner::started,       m_repositoryOverview, &RepositoryOverviewWidget::onUnitTestStarted);
		connect(m_unitTestRunner, &UnitTestRunner::finished,      m_repositoryOverview, &RepositoryOverviewWidget::onUnitTestFinished);
		connect(m_unitTestRunner, &UnitTestRunner::noExecutables, m_repositoryOverview, &RepositoryOverviewWidget::onUnitTestNoExecutables);
		// build runner -> build-result dialog + per-repo card collision lock + status bar
		connect(m_buildRunner, &BuildRunner::started, this, [this](const QString& path) {
			refreshCardLock(path, true, m_unitTestRunner->isRunning(path), m_queueActiveRepos.contains(path));
			updateStatusIndicators();
			});
		connect(m_buildRunner, &BuildRunner::finished, this, [this](const QString& path, int result, const QString&) {
			QString text = "ok", color = "#2ecc40";                 // green
			if (result == RepositoryJobQueue::ResultCanceled) { text = "cancel"; color = "#ff851b"; }  // orange
			else if (result != RepositoryJobQueue::ResultSuccess) { text = "fail"; color = "#ff4136"; } // red
			m_buildResultDialog->setResult(QDir(path).dirName(), path, text, color);
			m_buildResultDialog->popup();   // reopens if the user closed it
			refreshCardLock(path, false, m_unitTestRunner->isRunning(path), m_queueActiveRepos.contains(path));
			updateStatusIndicators();
			});
		connect(m_unitTestRunner, &UnitTestRunner::started, this, [this](const QString& path) {
			refreshCardLock(path, m_buildRunner->isRunning(path), true, m_queueActiveRepos.contains(path));
			});
		connect(m_unitTestRunner, &UnitTestRunner::finished, this,
			[this](const QString& path, int result, const QString&, const QVector<UnitTestSuiteResult>& suites) {
			QString text = "pass", color = "#2ecc40";                 // green
			if (result == RepositoryJobQueue::ResultCanceled) { text = "cancel"; color = "#ff851b"; }  // orange
			else if (result != RepositoryJobQueue::ResultSuccess) { text = "fail"; color = "#ff4136"; } // red

			QVector<JobResultDialog::SubItem> subs;
			for (const UnitTestSuiteResult& s : suites)
			{
				JobResultDialog::SubItem si;
				si.key = s.suiteName;
				si.label = s.exeName.isEmpty() ? (s.suiteName + " (no exe)") : (s.suiteName + " — " + s.exeName);
				if (s.result == RepositoryJobQueue::ResultSuccess)      { si.resultText = "pass";   si.color = "#2ecc40"; }
				else if (s.result == RepositoryJobQueue::ResultCanceled){ si.resultText = "cancel"; si.color = "#ff851b"; }
				else                                                    { si.resultText = "fail";   si.color = "#ff4136"; }
				subs.push_back(si);
			}

			m_testResultDialog->setResult(QDir(path).dirName(), path, text, color, subs);
			m_testResultDialog->popup();   // reopens if the user closed it
			refreshCardLock(path, m_buildRunner->isRunning(path), false, m_queueActiveRepos.contains(path));
			});
		connect(m_unitTestRunner, &UnitTestRunner::noExecutables, this, [this](const QString& path) {
			m_testResultDialog->setResult(QDir(path).dirName(), path, "error", "#ff4136");   // red
			m_testResultDialog->popup();
			refreshCardLock(path, m_buildRunner->isRunning(path), false, m_queueActiveRepos.contains(path));
			QMessageBox::warning(this, "No unittest executables",
				"No unittest executables found for " + QDir(path).dirName()
				+ " — build it first (or ensure build/Release/*.exe exist).");
			});
		connect(m_jobQueue, &RepositoryJobQueue::jobStarted, this, [this](const QString& path, int jobType, int index, int total) {
			m_queueActiveRepos.insert(path);
			m_repositoryOverview->onJobStarted(path, jobType);
			refreshCardLock(path, m_buildRunner->isRunning(path), m_unitTestRunner->isRunning(path), true);
			m_statusLabel->setText(QString("%1 %2/%3: %4")
				.arg(RepositoryJobQueue::jobVerb((RepositoryJobQueue::JobType)jobType)).arg(index).arg(total)
				.arg(QDir(path).dirName()));
			m_statusProgress->setRange(0, total);
			m_statusProgress->setValue(index - 1);
			m_statusProgress->show();
			m_cancelButton->show();
			m_cancelButton->setEnabled(true);
			});
		connect(m_jobQueue, &RepositoryJobQueue::jobFinished, this, [this](const QString& path, int jobType, int result) {
			m_queueActiveRepos.remove(path);
			m_repositoryOverview->onJobFinished(path, jobType, result);
			refreshCardLock(path, m_buildRunner->isRunning(path), m_unitTestRunner->isRunning(path), false);
			m_statusProgress->setValue(m_statusProgress->value() + 1);
			});
		connect(m_jobQueue, &RepositoryJobQueue::allJobsFinished, this, [this](bool canceled) {
			m_statusLabel->setText(canceled ? "Canceled" : "Done");
			m_statusProgress->hide();
			m_queueActiveRepos.clear();
			updateStatusIndicators();   // keeps Cancel visible if parallel builds are still running
			});
		// overview -> MainWindow (individual card buttons)
		connect(m_repositoryOverview, &RepositoryOverviewWidget::actionRequested, this, &MainWindow::onRepoActionRequested);
		connect(m_repositoryOverview, &RepositoryOverviewWidget::buildRequested, this, &MainWindow::onRepoBuildRequested);
		connect(m_repositoryOverview, &RepositoryOverviewWidget::unitTestRequested, this, &MainWindow::onRepoUnitTestRequested);
		connect(m_repositoryOverview, &RepositoryOverviewWidget::terminateRequested, this, [this](const QString& path) {
			// Build and unittest are mutually exclusive per repo; try both — the one not running is a no-op.
			if (m_buildRunner->isRunning(path))
				m_buildRunner->cancel(path);
			if (m_unitTestRunner->isRunning(path))
				m_unitTestRunner->cancel(path);
			});
		connect(m_repositoryOverview, &RepositoryOverviewWidget::showTestLogRequested, this, [this](const QString& path) {
			if (!m_testLogWindow) m_testLogWindow = new TextLogWindow();
			m_testLogWindow->showLog("Unittest log — " + QDir(path).dirName(), m_repositoryOverview->infoFor(path).unitTestLog);
			});
		// settings dialog -> rebuild page 2 + re-read the parallel-build cap
		connect(m_settingsDialog, &SettingsDialog::settingsSaved, m_repositoryOverview, &RepositoryOverviewWidget::reload);
		connect(m_settingsDialog, &SettingsDialog::settingsSaved, this, [this]() {
			m_buildRunner->setMaxThreads(Resources::getMaxBuildThreads());
			});
		// ribbon tab-2 group buttons
		auto& rb = RibbonImpl::getRepositoryButtons();
		connect(rb.refreshStatus, &QPushButton::clicked, this, [this]() {
			if (isAnyWorkRunning()) return;
			QVector<RepositoryJobQueue::Job> jobs;
			for (const auto& e : Resources::getLoadSaveProjects().projects)     // R10: refresh ALL repos, not only checked
				jobs.push_back({ e.path, RepositoryJobQueue::JobType::RefreshStatus, {} });
			m_jobQueue->enqueue(jobs);
			});
		connect(rb.openFolders, &QPushButton::clicked, this, [this]() {         // immediate, not queued
			for (const QString& p : m_repositoryOverview->checkedRepoPaths())
				Utilities::openFolderInExplorer(p);
			});
		connect(rb.updateTemplates, &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::UpdateTemplate); });
		connect(rb.pull,     &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::Pull); });
		connect(rb.push,     &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::Push); });
		connect(rb.commit,   &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::Commit); });
		connect(rb.discard,  &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::Discard); });
		connect(rb.build,    &QPushButton::clicked, this, [this]() { runGroupBuild(); });
		connect(rb.clean,    &QPushButton::clicked, this, [this]() { runGroupAction(RepositoryJobQueue::JobType::Clean); });
		connect(rb.unitTest, &QPushButton::clicked, this, [this]() { runGroupUnitTest(); });
		connect(rb.terminateAll, &QPushButton::clicked, this, [this]() {
			// Group stop: kill every in-flight parallel build and unittest, and cancel the sequential queue.
			m_buildRunner->cancelAll();
			m_unitTestRunner->cancelAll();
			if (m_jobQueue) m_jobQueue->cancel();
			m_statusLabel->setText("Canceling...");
			});
	}

	MainWindow::GroupWarnChoice MainWindow::askGroupWarning(const QString& title, const QString& intro, const QStringList& repoNames)
	{
		QMessageBox box(QMessageBox::Warning, title, intro + "\n\n- " + repoNames.join("\n- "), QMessageBox::NoButton, this);
		QPushButton* proceed = box.addButton("Proceed for all", QMessageBox::AcceptRole);
		QPushButton* skip = box.addButton("Skip affected", QMessageBox::ActionRole);
		box.addButton("Cancel", QMessageBox::RejectRole);
		box.exec();
		if (box.clickedButton() == proceed) return GroupWarnChoice::ProceedAll;
		if (box.clickedButton() == skip)    return GroupWarnChoice::SkipAffected;
		return GroupWarnChoice::Cancel;
	}

	bool MainWindow::askCommitMessage(QString& messageOut)
	{
		QString tplVersion;
		Utilities::readTemplateVersion(Resources::getCurrentTemplateAbsSourcePath() + "/CMakeLists.txt", tplVersion);
		QString def = "~ Update Library Template";
		if (!tplVersion.isEmpty())
			def += " " + tplVersion;
		bool ok = false;
		messageOut = QInputDialog::getText(this, "Commit message", "Enter the commit message:", QLineEdit::Normal, def, &ok);
		return ok && !messageOut.trimmed().isEmpty();
	}

	bool MainWindow::isAnyWorkRunning(bool warn)
	{
		const bool running = (m_workerThread && m_workerThread->isRunning()) || (m_jobQueue && m_jobQueue->isBusy());
		if (running && warn)
			Logging::getLogger().logWarning("An operation is already running");
		return running;
	}

	MainWindow::RepoOp MainWindow::opForJobType(RepositoryJobQueue::JobType type)
	{
		switch (type)
		{
		case RepositoryJobQueue::JobType::UpdateTemplate: return RepoOp::UpdateTemplate;
		case RepositoryJobQueue::JobType::Pull:           return RepoOp::Pull;
		case RepositoryJobQueue::JobType::Push:           return RepoOp::Push;
		case RepositoryJobQueue::JobType::Commit:         return RepoOp::Commit;
		case RepositoryJobQueue::JobType::Discard:        return RepoOp::Discard;
		case RepositoryJobQueue::JobType::Clean:          return RepoOp::Clean;
		case RepositoryJobQueue::JobType::RefreshStatus:  return RepoOp::Pull;   // read-only; never used for collisions
		}
		return RepoOp::Pull;
	}

	bool MainWindow::repoCollision(const QString& path, RepoOp op) const
	{
		const bool building    = m_buildRunner->isRunning(path);
		const bool testing     = m_unitTestRunner->isRunning(path);
		const bool queueActive = m_queueActiveRepos.contains(path);

		// An active queue job on this repo blocks any new mutating op on this repo.
		if (queueActive)
			return true;
		switch (op)
		{
		case RepoOp::Build:          return building || testing;   // dup build, or exes in use by tests
		case RepoOp::Clean:          return building || testing;   // touches the build tree in use
		case RepoOp::UnitTest:       return building || testing;   // dup test, or building same repo
		case RepoOp::UpdateTemplate: return building;              // rewrites the source/build tree
		case RepoOp::Pull:
		case RepoOp::Push:
		case RepoOp::Commit:
		case RepoOp::Discard:        return false;                 // git ops collide only with a queue job
		}
		return false;
	}

	void MainWindow::refreshCardLock(const QString& path, bool building, bool testing, bool queueActive)
	{
		m_repositoryOverview->applyCollisionLock(path, building, testing, queueActive);
	}

	void MainWindow::updateStatusIndicators()
	{
		const int builds = m_buildRunner->activeCount();
		if (builds > 0)
			m_buildStatusLabel->setText(QString("Building %1 repositories…").arg(builds));
		else
			m_buildStatusLabel->clear();

		const bool busy = builds > 0 || (m_jobQueue && m_jobQueue->isBusy());
		m_cancelButton->setVisible(busy);
		if (busy)
			m_cancelButton->setEnabled(true);
	}

	void MainWindow::onRepoActionRequested(const QString& path, RepositoryJobQueue::JobType type)
	{
		const QString name = QDir(path).dirName();
		if (repoCollision(path, opForJobType(type)))
		{
			Logging::getLogger().logWarning((name + ": an operation on this repository is already running — skipped").toStdString());
			return;
		}

		const RepositoryInfo info = m_repositoryOverview->infoFor(path);
		QString commitMessage;

		switch (type)
		{
			case RepositoryJobQueue::JobType::UpdateTemplate:
			{
				// Skip-if-current: if the repo already carries the downloaded template
				// version, there is nothing to do.
				QString repoVer, downloadedVer;
				const bool repoOk = Utilities::readTemplateVersion(path + "/CMakeLists.txt", repoVer);
				const bool downloadedOk = Utilities::readTemplateVersion(
					Resources::getCurrentTemplateAbsSourcePath() + "/CMakeLists.txt", downloadedVer);
				if (repoOk && downloadedOk && repoVer == downloadedVer)
				{
					QMessageBox::information(this, "Already up to date",
						"Already up to date — " + name + " is on template " + repoVer + ".");
					return;
				}
				if (Utilities::gitIsDirty(path))       // R11: fresh check for a single repo
				{
					if (QMessageBox::warning(this, "Uncommitted changes",
						name + " has uncommitted changes.\n\nThe library template should only be updated on a clean repository.\nUpdate anyway?",
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
						return;
				}
				break;
			}
			case RepositoryJobQueue::JobType::Commit:
			{
				if (!Utilities::gitIsDirty(path))
				{
					QMessageBox::information(this, "Nothing to commit",
						name + " has no changes to commit.");
					return;
				}
				const bool failed = info.buildStatus == RepositoryInfo::BuildStatus::Error
					|| info.testStatus == RepositoryInfo::TestStatus::Failed
					|| info.testStatus == RepositoryInfo::TestStatus::Error;
				if (failed)
				{
					if (QMessageBox::warning(this, "Build/Unittest failed",
						name + " has a failed build or unittest run.\n\nCommit anyway?",
						QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
						return;
				}
				if (!askCommitMessage(commitMessage))
					return;
				break;
			}
			case RepositoryJobQueue::JobType::Discard:
			{
				if (QMessageBox::warning(this, "Discard changes",
					"This reverts ALL uncommitted changes in " + name + " and deletes untracked files (git reset --hard + git clean -fd).\n\nContinue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
					return;
				break;
			}
			case RepositoryJobQueue::JobType::Push:
			{
				bool commandOk = false;
				const bool hasUnpushed = Utilities::gitHasUnpushedCommits(path, &commandOk);
				if (commandOk && !hasUnpushed)
				{
					QMessageBox::information(this, "Nothing to push",
						name + " has nothing to push.");
					return;
				}
				break;
			}
			case RepositoryJobQueue::JobType::Clean:
			{
				if (QMessageBox::warning(this, "Clean build folder",
					"This deletes the build folder of " + name + ".\n\nContinue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
					return;
				break;
			}
			default:
				break;
		}

		m_jobQueue->enqueue({ { path, type, commitMessage } });
	}

	void MainWindow::onRepoBuildRequested(const QString& path)
	{
		const QString name = QDir(path).dirName();
		if (repoCollision(path, RepoOp::Build))
		{
			Logging::getLogger().logWarning((name + ": build skipped — an operation on this repository is already running").toStdString());
			return;
		}
		// Fresh single-repo build run: reset the result list + this repo's build fields.
		m_buildResultDialog->beginRun();
		m_repositoryOverview->resetRunFields(path, true);
		m_buildRunner->run(path);   // off-queue; parallel per repo, capped by maxBuildThreads
	}

	void MainWindow::runGroupAction(RepositoryJobQueue::JobType type)
	{
		QStringList repos = m_repositoryOverview->checkedRepoPaths();
		if (repos.isEmpty())
		{
			QMessageBox::information(this, "Group action",
				"No repositories are selected for group actions.\nEnable the checkbox on the repositories you want to include.");
			return;
		}

		auto namesOf = [this](const QStringList& paths) {
			QStringList out;
			for (const QString& p : paths) out.push_back(QDir(p).dirName());
			return out;
		};

		// Per-repo collision: drop only the repos where this op collides with in-progress
		// work on that same repo; the survivors still run through the pre-check dialogs.
		{
			const RepoOp op = opForJobType(type);
			QStringList colliding;
			for (const QString& p : repos)
				if (repoCollision(p, op))
					colliding.push_back(p);
			if (!colliding.isEmpty())
			{
				for (const QString& p : colliding) repos.removeAll(p);
				Logging::getLogger().logWarning("Skipping repositories with an operation already running: "
					+ namesOf(colliding).join(", ").toStdString());
				if (repos.isEmpty()) return;
			}
		}

		// Capability filter: git-only ops need a git repo; push/pull additionally need a remote.
		{
			const bool needsGit    = (type == RepositoryJobQueue::JobType::Pull
								   || type == RepositoryJobQueue::JobType::Push
								   || type == RepositoryJobQueue::JobType::Commit
								   || type == RepositoryJobQueue::JobType::Discard);
			const bool needsRemote = (type == RepositoryJobQueue::JobType::Pull
								   || type == RepositoryJobQueue::JobType::Push);
			if (needsGit)
			{
				QStringList skipped;
				QString reason;
				for (const QString& p : repos)
				{
					const RepositoryInfo info = m_repositoryOverview->infoFor(p);
					if (!info.isGitRepo)                     { skipped.push_back(p); reason = "not a git repository"; }
					else if (needsRemote && !info.hasRemote) { skipped.push_back(p); reason = "no configured remote"; }
				}
				if (!skipped.isEmpty())
				{
					for (const QString& p : skipped) repos.removeAll(p);
					QMessageBox::information(this, "Skipped repositories",
						"The following repositories were skipped (" + reason + "):\n\n- "
						+ namesOf(skipped).join("\n- "));
					if (repos.isEmpty()) return;
				}
			}
		}

		QString commitMessage;

		switch (type)
		{
			case RepositoryJobQueue::JobType::UpdateTemplate:
			{
				// Skip-if-current first: drop repos already on the downloaded template
				// version, then run the dirty pre-check on the ones that remain.
				QString downloadedVer;
				const bool downloadedOk = Utilities::readTemplateVersion(
					Resources::getCurrentTemplateAbsSourcePath() + "/CMakeLists.txt", downloadedVer);
				if (downloadedOk)
				{
					QStringList skipped;
					for (const QString& p : repos)
					{
						QString repoVer;
						if (Utilities::readTemplateVersion(p + "/CMakeLists.txt", repoVer) && repoVer == downloadedVer)
							skipped.push_back(p);
					}
					if (!skipped.isEmpty())
					{
						QMessageBox::information(this, "Already up to date",
							"These repositories are already on template " + downloadedVer + " and will be skipped:\n\n- "
								+ namesOf(skipped).join("\n- "));
						for (const QString& p : skipped) repos.removeAll(p);
						if (repos.isEmpty()) return;
					}
				}
				QStringList affected;
				for (const QString& p : repos)
					if (m_repositoryOverview->infoFor(p).hasUncommittedChanges)
						affected.push_back(p);
				if (!affected.isEmpty())
				{
					const GroupWarnChoice choice = askGroupWarning("Uncommitted changes",
						"These repositories have uncommitted changes. The library template should only be updated on clean repositories.",
						namesOf(affected));
					if (choice == GroupWarnChoice::Cancel) return;
					if (choice == GroupWarnChoice::SkipAffected)
					{
						for (const QString& p : affected) repos.removeAll(p);
						if (repos.isEmpty()) return;
					}
				}
				break;
			}
			case RepositoryJobQueue::JobType::Commit:
			{
				// Drop repos with no changes before anything else; if none remain, stop.
				QStringList clean;
				for (const QString& p : repos)
					if (!Utilities::gitIsDirty(p))
						clean.push_back(p);
				if (!clean.isEmpty())
				{
					for (const QString& p : clean) repos.removeAll(p);
					if (repos.isEmpty())
					{
						QMessageBox::information(this, "Nothing to commit",
							"None of the selected repositories have changes to commit.");
						return;
					}
				}
				QStringList affected;
				for (const QString& p : repos)
				{
					const RepositoryInfo i = m_repositoryOverview->infoFor(p);
					if (i.buildStatus == RepositoryInfo::BuildStatus::Error
						|| i.testStatus == RepositoryInfo::TestStatus::Failed
						|| i.testStatus == RepositoryInfo::TestStatus::Error)
						affected.push_back(p);
				}
				if (!affected.isEmpty())
				{
					const GroupWarnChoice choice = askGroupWarning("Build/Unittest failed",
						"These repositories have a failed build or unittest run.",
						namesOf(affected));
					if (choice == GroupWarnChoice::Cancel) return;
					if (choice == GroupWarnChoice::SkipAffected)
					{
						for (const QString& p : affected) repos.removeAll(p);
						if (repos.isEmpty()) return;
					}
				}
				if (!askCommitMessage(commitMessage))
					return;
				break;
			}
			case RepositoryJobQueue::JobType::Discard:
			{
				// All selected repos are "affected"; skip-affected is meaningless -> plain Yes/No.
				if (QMessageBox::warning(this, "Discard changes",
					"This reverts ALL uncommitted changes and deletes untracked files (git reset --hard + git clean -fd) in these repositories:\n\n- "
						+ namesOf(repos).join("\n- ") + "\n\nContinue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
					return;
				break;
			}
			case RepositoryJobQueue::JobType::Push:
			{
				// Keep repos where a push is allowed or the state is undetermined.
				QStringList nothing;
				for (const QString& p : repos)
				{
					bool commandOk = false;
					const bool hasUnpushed = Utilities::gitHasUnpushedCommits(p, &commandOk);
					if (commandOk && !hasUnpushed)
						nothing.push_back(p);
				}
				if (!nothing.isEmpty())
				{
					for (const QString& p : nothing) repos.removeAll(p);
					if (repos.isEmpty())
					{
						QMessageBox::information(this, "Nothing to push",
							"None of the selected repositories have commits to push.");
						return;
					}
				}
				break;
			}
			case RepositoryJobQueue::JobType::Clean:
			{
				if (QMessageBox::warning(this, "Clean build folders",
					"This deletes the build folder of the " + QString::number(repos.size())
						+ " selected repositories.\n\nContinue?",
					QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
					return;
				break;
			}
			default:
				break;
		}

		QVector<RepositoryJobQueue::Job> jobs;
		for (const QString& p : repos)
			jobs.push_back({ p, type, commitMessage });
		m_jobQueue->enqueue(jobs);
	}

	void MainWindow::runGroupBuild()
	{
		QStringList repos = m_repositoryOverview->checkedRepoPaths();
		if (repos.isEmpty())
		{
			QMessageBox::information(this, "Group action",
				"No repositories are selected for group actions.\nEnable the checkbox on the repositories you want to include.");
			return;
		}

		// Parallel run: "start" is the moment the group action fires -> reset the list once here.
		m_buildResultDialog->beginRun();

		QStringList skipped;
		for (const QString& p : repos)
		{
			if (repoCollision(p, RepoOp::Build))
			{
				skipped.push_back(QDir(p).dirName());
				continue;
			}
			m_repositoryOverview->resetRunFields(p, true);
			m_buildRunner->run(p);   // pool caps concurrency; extra repos wait for a free slot
		}
		if (!skipped.isEmpty())
			Logging::getLogger().logWarning("Skipping builds (operation already running): "
				+ skipped.join(", ").toStdString());
	}

	bool MainWindow::canRunUnitTest(const QString& path, bool warn)
	{
		if (!repoCollision(path, RepoOp::UnitTest))
			return true;
		if (warn)
		{
			if (m_buildRunner->isRunning(path))
				QMessageBox::warning(this, "Unittest blocked",
					QDir(path).dirName() + " is currently building.\nWait for the build to finish before running its unittests.");
			else
				Logging::getLogger().logWarning(("Unittests blocked — an operation is already running for " + QDir(path).dirName()).toStdString());
		}
		return false;
	}

	void MainWindow::onRepoUnitTestRequested(const QString& path)
	{
		if (!canRunUnitTest(path, true))
			return;
		// Fresh single-repo unittest run: reset the result list + this repo's test fields.
		m_testResultDialog->beginRun();
		m_repositoryOverview->resetRunFields(path, false);
		m_unitTestRunner->run(path);   // off-queue; parallel per repo
	}

	void MainWindow::runGroupUnitTest()
	{
		QStringList repos = m_repositoryOverview->checkedRepoPaths();
		if (repos.isEmpty())
		{
			QMessageBox::information(this, "Group action",
				"No repositories are selected for group actions.\nEnable the checkbox on the repositories you want to include.");
			return;
		}

		// Parallel run: "start" is the moment the group action fires -> reset the list once here.
		m_testResultDialog->beginRun();

		QStringList skipped;
		for (const QString& p : repos)
		{
			if (canRunUnitTest(p, false))
			{
				m_repositoryOverview->resetRunFields(p, false);
				m_unitTestRunner->run(p);
			}
			else
				skipped.push_back(QDir(p).dirName());
		}
		if (!skipped.isEmpty())
			Logging::getLogger().logWarning("Skipping unittests (building or already running): "
				+ skipped.join(", ").toStdString());
	}
}
