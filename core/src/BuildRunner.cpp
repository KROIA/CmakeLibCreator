#include "BuildRunner.h"
#include "RepositoryJobQueue.h"
#include "Logging.h"

#include <QThread>
#include <QProcess>
#include <string>

namespace CLC
{
    BuildRunner::BuildRunner(QObject* parent)
        : QObject(parent)
    {
    }

    BuildRunner::~BuildRunner()
    {
        shutdown(5000);
    }

    void BuildRunner::run(const QString& repoPath)
    {
        QMutexLocker lock(&m_mutex);
        if (m_active.contains(repoPath) || m_pendingSet.contains(repoPath))
            return;                                  // refuse a duplicate run
        m_canceling = false;                         // a fresh run re-arms the pool
        if (m_active.size() < m_maxThreads)
            startLocked(repoPath);
        else
        {
            m_pendingQueue.append(repoPath);
            m_pendingSet.insert(repoPath);
        }
    }

    void BuildRunner::startLocked(const QString& repoPath)
    {
        m_active.insert(repoPath);

        QThread* thread = new QThread();
        m_threads.insert(repoPath, thread);

        // No context object: the functor runs as a direct connection on the worker
        // thread, so executeBuild() runs off the GUI thread. Signals reach GUI slots
        // via queued AutoConnection because this runner lives on the GUI thread.
        QObject::connect(thread, &QThread::started, [this, repoPath, thread]() {
            executeBuild(repoPath);
            thread->exit();
        });
        QObject::connect(thread, &QThread::finished, this, [this, repoPath, thread]() {
            {
                QMutexLocker lock(&m_mutex);
                m_active.remove(repoPath);
                m_threads.remove(repoPath);
            }
            thread->deleteLater();
            startNextPending();
        });
        thread->start();
    }

    void BuildRunner::startNextPending()
    {
        QMutexLocker lock(&m_mutex);
        while (!m_pendingQueue.isEmpty() && m_active.size() < m_maxThreads)
        {
            const QString next = m_pendingQueue.takeFirst();
            m_pendingSet.remove(next);
            startLocked(next);
        }
    }

    bool BuildRunner::isRunning(const QString& repoPath) const
    {
        QMutexLocker lock(&m_mutex);
        return m_active.contains(repoPath);
    }

    int BuildRunner::activeCount() const
    {
        QMutexLocker lock(&m_mutex);
        return m_active.size();
    }

    void BuildRunner::setMaxThreads(int n)
    {
        {
            QMutexLocker lock(&m_mutex);
            m_maxThreads = n < 1 ? 1 : n;
        }
        startNextPending();   // a larger cap may free up slots for pending builds
    }

    void BuildRunner::executeBuild(const QString& repoPath)
    {
        emit started(repoPath);
        QString log;
        const int result = runBuildProcess(repoPath, &log);
        emit finished(repoPath, result, log);
    }

    int BuildRunner::runBuildProcess(const QString& repoPath, QString* capturedOutput)
    {
        // QProcess working dir avoids an unquoted cd (paths-with-spaces safe).
        QProcess proc;
        proc.setProgram("cmd");
        proc.setArguments({ "/c", "build.bat" });
        proc.setWorkingDirectory(repoPath);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        if (!proc.waitForStarted(10000))
        {
            Logging::getLogger().logError("Failed to start build.bat in: " + repoPath.toStdString());
            return RepositoryJobQueue::ResultFailed;
        }

        const qint64 pid = proc.processId();
        {
            QMutexLocker lock(&m_mutex);
            m_pids.insert(repoPath, pid);
        }

        QString buffer;
        while (proc.state() != QProcess::NotRunning)
        {
            proc.waitForFinished(100);   // pumps process I/O; returns on timeout or exit
            const QString chunk = QString::fromLocal8Bit(proc.readAll());
            if (!chunk.isEmpty())
            {
                buffer += chunk;
                int nl;
                while ((nl = buffer.indexOf('\n')) >= 0)
                {
                    Logging::getLogger().logInfo(buffer.left(nl).trimmed().toStdString());
                    if (capturedOutput) *capturedOutput += buffer.left(nl + 1);
                    buffer.remove(0, nl + 1);
                }
            }
            // cancelAll() issues taskkill on this pid; the loop exits when the process dies
        }
        const QString rest = buffer + QString::fromLocal8Bit(proc.readAll());
        if (!rest.trimmed().isEmpty())
        {
            Logging::getLogger().logInfo(rest.trimmed().toStdString());
            if (capturedOutput) *capturedOutput += rest;
        }

        bool perRepoCanceled = false;
        {
            QMutexLocker lock(&m_mutex);
            m_pids.remove(repoPath);
            perRepoCanceled = m_canceledRepos.remove(repoPath);
        }

        if (m_canceling.load() || perRepoCanceled)
            return RepositoryJobQueue::ResultCanceled;
        if (proc.exitStatus() != QProcess::NormalExit)
            return RepositoryJobQueue::ResultFailed;
        return proc.exitCode() == 0 ? RepositoryJobQueue::ResultSuccess : RepositoryJobQueue::ResultFailed;
    }

    void BuildRunner::cancel(const QString& repoPath)
    {
        qint64 pid = 0;
        bool wasPending = false;
        {
            QMutexLocker lock(&m_mutex);
            if (m_pendingSet.contains(repoPath))
            {
                m_pendingQueue.removeAll(repoPath);
                m_pendingSet.remove(repoPath);
                wasPending = true;
            }
            if (m_active.contains(repoPath))
            {
                pid = m_pids.value(repoPath, 0);
                m_canceledRepos.insert(repoPath);   // consumed by runBuildProcess to mark Canceled
            }
        }
        if (pid != 0)
        {
            Logging::getLogger().logInfo("Canceling build: killing process tree " + std::to_string(pid));
            QProcess::startDetached("taskkill", { "/PID", QString::number(pid), "/T", "/F" });
        }
        if (wasPending)
            emit finished(repoPath, RepositoryJobQueue::ResultCanceled, QString());
    }

    void BuildRunner::cancelAll()
    {
        m_canceling = true;

        QList<qint64> pids;
        QList<QString> pendings;
        {
            QMutexLocker lock(&m_mutex);
            pids = m_pids.values();
            pendings = m_pendingQueue;
            m_pendingQueue.clear();
            m_pendingSet.clear();
        }
        for (const qint64 pid : pids)
        {
            if (pid == 0) continue;
            Logging::getLogger().logInfo("Canceling build: killing process tree " + std::to_string(pid));
            QProcess::startDetached("taskkill", { "/PID", QString::number(pid), "/T", "/F" });
        }
        // Dropped pending builds never started, so report them as canceled to clear the UI.
        for (const QString& p : pendings)
            emit finished(p, RepositoryJobQueue::ResultCanceled, QString());
    }

    void BuildRunner::shutdown(int timeoutMs)
    {
        cancelAll();
        QList<QThread*> threads;
        {
            QMutexLocker lock(&m_mutex);
            threads = m_threads.values();
        }
        for (QThread* t : threads)
            if (t) t->wait(timeoutMs);
    }
}
