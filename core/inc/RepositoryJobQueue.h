#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RepositoryInfo.h"
#include <QObject>
#include <QVector>
#include <QMutex>
#include <atomic>

class QThread;

namespace CLC
{
    // Runs repository jobs strictly sequentially on a worker thread.
    // External processes are cancelable via taskkill on the whole process tree.
    class RepositoryJobQueue : public QObject
    {
        Q_OBJECT
    public:
        enum class JobType { Pull, Push, Commit, Discard, UpdateTemplate, Clean, RefreshStatus };
        struct Job
        {
            QString repoPath;
            JobType type = JobType::RefreshStatus;
            QString commitMessage; // Commit only
        };
        static constexpr int ResultSuccess = 0;
        static constexpr int ResultFailed = 1;
        static constexpr int ResultCanceled = 2;

        explicit RepositoryJobQueue(QObject* parent = nullptr);
        ~RepositoryJobQueue();

        void enqueue(const QVector<Job>& jobs); // starts the worker if idle
        void cancel();                          // kills current process tree, drops pending jobs
        bool isBusy() const;
        void shutdown(int timeoutMs);           // cancel + wait for worker (for closeEvent)

        static QString jobVerb(JobType type);   // "Building", "Pulling", ... for the status bar

    signals:
        // jobType is the JobType cast to int (avoids extra metatype registration)
        void jobStarted(const QString& repoPath, int jobType, int index, int total);
        void jobFinished(const QString& repoPath, int jobType, int result); // Result* constants
        void statusRefreshed(const QString& repoPath, const CLC::RepositoryInfo& info);
        void templateUpdated(const QString& repoPath, const QString& previousTemplateVersion);
        void allJobsFinished(bool canceled);

    private:
        void workerLoop();                       // runs on m_thread
        int  runJob(const Job& job);             // dispatch, returns Result*
        // rawExitCode (optional): receives the raw process exit code on NormalExit so the
        // inverted unittest convention (NormalExit && code != 0 == PASS) can be evaluated.
        int  runProcessJob(const QString& program, const QStringList& args,
                           const QString& workingDir, QString* capturedOutput,
                           int* rawExitCode = nullptr);
        int  runPull(const Job&);
        int  runPush(const Job&);
        int  runCommit(const Job&);
        int  runDiscard(const Job&);
        int  runUpdateTemplate(const Job&);
        int  runClean(const Job&);
        void refreshStatus(const QString& repoPath); // gathers info + emits statusRefreshed

        mutable QMutex m_mutex;
        QVector<Job> m_pending;
        int m_totalJobs = 0;   // total of current run (for progress)
        int m_doneJobs = 0;
        bool m_busy = false;
        QThread* m_thread = nullptr;
        std::atomic<bool> m_cancelRequested{ false };
        std::atomic<qint64> m_currentPid{ 0 };
    };
}
