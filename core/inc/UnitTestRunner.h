#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QObject>
#include <QSet>
#include <QMap>
#include <QMutex>

class QThread;

namespace CLC
{
    // Runs a repository's unittests off the sequential job queue: one worker thread
    // per repo path, so different repos test in parallel. A repo already testing is
    // refused (mutex-guarded active set). Result uses the queue convention
    // (0 success, 1 failed, 2 canceled) with the inverted KROIA exit code.
    class UnitTestRunner : public QObject
    {
        Q_OBJECT
    public:
        explicit UnitTestRunner(QObject* parent = nullptr);
        ~UnitTestRunner();

        void run(const QString& repoPath);                 // no-op if already running for this path
        bool isRunning(const QString& repoPath) const;

    signals:
        void started(const QString& repoPath);
        void finished(const QString& repoPath, int result, const QString& log);
        void noExecutables(const QString& repoPath);       // folder present but no build/Release/*.exe

    private:
        void executeTests(const QString& repoPath);        // runs on the per-repo worker thread
        int  runProcess(const QString& program, const QString& workingDir,
                        QString* capturedOutput, int* rawExitCode);

        mutable QMutex m_mutex;
        QSet<QString> m_active;              // repo paths currently testing
        QMap<QString, QThread*> m_threads;   // repo path -> its worker thread
    };
}
