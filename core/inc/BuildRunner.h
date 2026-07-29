#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QObject>
#include <QSet>
#include <QMap>
#include <QList>
#include <QMutex>
#include <atomic>

class QThread;

namespace CLC
{
    // Parallel build pool: each repository builds on its own worker thread
    // (cmd /c build.bat), capped at maxThreads. Extra requests wait for a free
    // slot. A repo already active or pending is refused a duplicate run. Result
    // uses the queue convention (0 success, 1 failed, 2 canceled).
    class BuildRunner : public QObject
    {
        Q_OBJECT
    public:
        explicit BuildRunner(QObject* parent = nullptr);
        ~BuildRunner();

        void run(const QString& repoPath);            // no-op if already active/pending
        bool isRunning(const QString& repoPath) const;
        int  activeCount() const;
        void setMaxThreads(int n);                    // clamped to >= 1; fills freed slots
        void cancel(const QString& repoPath);         // taskkill just this repo's build tree
        void cancelAll();                             // kill in-flight trees, drop pending
        void shutdown(int timeoutMs);                 // cancelAll + bounded join (closeEvent)

    signals:
        void started(const QString& repoPath);
        void finished(const QString& repoPath, int result, const QString& log);

    private:
        void startLocked(const QString& repoPath);    // caller holds m_mutex
        void startNextPending();                      // locks internally
        void executeBuild(const QString& repoPath);   // runs on the worker thread
        int  runBuildProcess(const QString& repoPath, QString* capturedOutput);

        mutable QMutex m_mutex;
        QSet<QString> m_active;                 // repos currently building
        QList<QString> m_pendingQueue;          // waiting for a free slot (ordered)
        QSet<QString> m_pendingSet;             // membership check for m_pendingQueue
        QMap<QString, QThread*> m_threads;      // repo path -> its worker thread
        QMap<QString, qint64> m_pids;           // repo path -> in-flight process id
        QSet<QString> m_canceledRepos;          // per-repo cancel flag consumed by runBuildProcess
        int m_maxThreads = 4;
        std::atomic<bool> m_canceling{ false };
    };
}
