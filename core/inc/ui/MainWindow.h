#pragma once
#include "CmakeLibraryCreator_base.h"
#include <QMainWindow>
#include <QTimer>
#include "ui_MainWindow.h"
#include "RibbonImpl.h"
#include "ProjectSettingsDialog.h"
#include "SettingsDialog.h"
#include "RepositoryJobQueue.h"
#include "RepositoryOverviewWidget.h"
#include "UnitTestRunner.h"
#include "BuildRunner.h"
#include "TextLogWindow.h"
#include "JobResultDialog.h"
#include <QSet>

class QTabWidget;
class QLabel;
class QProgressBar;
class QPushButton;

namespace CLC
{
	class MainWindow : public QMainWindow
	{
		Q_OBJECT

	public:
		MainWindow(QWidget* parent = nullptr);
		~MainWindow();

	private slots:
		void onOpenTemplatePath_clicked();
		void onDownloadTemplate_clicked();

		void onOpenExistingProject_clicked();
		void onSaveExistingProject_clicked();
		void onSaveAsNewProject_clicked();

		void on_actionVersion_triggered();
		void on_actionAbout_triggered();

		void on_actionSettings_triggered();
		void onExportDialogOkButtonClicked(const QVector<CheckBoxSelectionDialog::Element>& selectedItems);
	
		void disableUI();
		void enableUI();


		void signalInformation(const QString& title,
			const QString& text);
		void signalWarning(const QString& title,
			const QString& text);
		void signalCritical(const QString& title,
			const QString& text);
		//void onTimerTimeout();
	private:
		void threadFinished();

		bool loadProjectAsync(const QString& path);

		void closeEvent(QCloseEvent* event) override;

		enum class GroupWarnChoice { ProceedAll, SkipAffected, Cancel };
		GroupWarnChoice askGroupWarning(const QString& title, const QString& intro, const QStringList& repoNames);
		bool askCommitMessage(QString& messageOut);          // pre-filled "~ Update Library Template <ver>"
		// isAnyWorkRunning stays a GLOBAL guard for the legacy tab-1 worker-thread flows
		// (download template / open / save project). Repository per-repo flows use repoCollision instead.
		bool isAnyWorkRunning(bool warn = true);             // m_workerThread running || m_jobQueue->isBusy()

		// Per-repo collision model spanning queue + build pool + unittest runner.
		enum class RepoOp { UpdateTemplate, Pull, Push, Commit, Discard, Build, Clean, UnitTest };
		bool repoCollision(const QString& path, RepoOp op) const;   // true => refuse/disable on THIS repo
		static RepoOp opForJobType(RepositoryJobQueue::JobType type);
		void refreshCardLock(const QString& path, bool building, bool testing, bool queueActive);
		void updateStatusIndicators();                       // build-count label + cancel-button visibility

		void onRepoActionRequested(const QString& path, RepositoryJobQueue::JobType type);   // individual queue buttons
		void onRepoBuildRequested(const QString& path);      // individual card Build (off-queue, parallel pool)
		void runGroupAction(RepositoryJobQueue::JobType type);                               // ribbon tab-2 buttons
		void runGroupBuild();                                // ribbon tab-2 Build (off-queue, parallel pool)
		void onRepoUnitTestRequested(const QString& path);   // individual card Unittest (off-queue)
		void runGroupUnitTest();                             // ribbon tab-2 Unittest (off-queue)
		bool canRunUnitTest(const QString& path, bool warn); // guards via repoCollision(UnitTest)
		void setupCentralTabs();
		void setupStatusBar();
		void setupRepositoryConnections();


		Ui::MainWindow ui;
		RibbonImpl* m_ribbon;
		ProjectSettingsDialog * m_projectSettingsDialog;
		CheckBoxSelectionDialog* m_exportSettingsDialog;
		SettingsDialog * m_settingsDialog;
		bool m_existingProjectLoaded = false;
		bool m_closing = false;   // set in closeEvent to short-circuit tab-sync lambdas during teardown

		QThread *m_workerThread = nullptr;
		//QTimer m_timer;

		QTabWidget* m_centralTabs = nullptr;
		RepositoryOverviewWidget* m_repositoryOverview = nullptr;
		RepositoryJobQueue* m_jobQueue = nullptr;
		UnitTestRunner* m_unitTestRunner = nullptr;
		BuildRunner* m_buildRunner = nullptr;
		QSet<QString> m_queueActiveRepos;   // repo paths with an in-flight queue job
		TextLogWindow* m_testLogWindow = nullptr;
		JobResultDialog* m_buildResultDialog = nullptr;  // build-run result list popup
		JobResultDialog* m_testResultDialog = nullptr;   // unittest-run result list popup
		QLabel* m_statusLabel = nullptr;
		QLabel* m_buildStatusLabel = nullptr;   // "Building N repositories…" (parallel pool)
		QProgressBar* m_statusProgress = nullptr;
		QPushButton* m_cancelButton = nullptr;
	};
}
