#include "RepositoryJobQueue.h"
#include "Utilities.h"
#include "Resources.h"
#include "ProjectExporter.h"
#include "ProjectSettings.h"
#include "Logging.h"

#include <QThread>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <string>

namespace CLC
{
    RepositoryJobQueue::RepositoryJobQueue(QObject* parent)
        : QObject(parent)
    {
        qRegisterMetaType<CLC::RepositoryInfo>("CLC::RepositoryInfo");
    }

    RepositoryJobQueue::~RepositoryJobQueue()
    {
        shutdown(5000);
    }

    void RepositoryJobQueue::enqueue(const QVector<Job>& jobs)
    {
        QMutexLocker lock(&m_mutex);
        m_pending.append(jobs);
        m_totalJobs += jobs.size();
        if (m_busy)
            return;

        m_busy = true;
        m_cancelRequested = false;
        m_doneJobs = 0;

        m_thread = new QThread();
        // No context object: the functor runs as a direct connection on the emitting
        // (worker) thread, so workerLoop() executes off the GUI thread.
        QObject::connect(m_thread, &QThread::started, [this]() { workerLoop(); });
        QObject::connect(m_thread, &QThread::finished, this, [this]() {
            const bool canceled = m_cancelRequested;
            if (m_thread)
            {
                m_thread->deleteLater();
                m_thread = nullptr;
            }
            {
                QMutexLocker lock(&m_mutex);
                m_busy = false;
                m_pending.clear();
                m_totalJobs = 0;
                m_doneJobs = 0;
            }
            emit allJobsFinished(canceled);
        });
        m_thread->start();
    }

    void RepositoryJobQueue::workerLoop()
    {
        while (true)
        {
            Job job;
            {
                QMutexLocker lock(&m_mutex);
                if (m_pending.isEmpty())
                    break;
                job = m_pending.takeFirst();
            }

            if (m_cancelRequested)
            {
                // Drain the remaining queue, reporting each dropped job as canceled.
                emit jobFinished(job.repoPath, (int)job.type, ResultCanceled);
                continue;
            }

            emit jobStarted(job.repoPath, (int)job.type, m_doneJobs + 1, m_totalJobs);
            const int result = runJob(job);
            ++m_doneJobs;
            emit jobFinished(job.repoPath, (int)job.type, result);

            // Keep the GUI current with a single mechanism after any state-changing job.
            if (job.type != JobType::RefreshStatus)
                refreshStatus(job.repoPath);
        }
        m_thread->exit();
    }

    int RepositoryJobQueue::runJob(const Job& job)
    {
        switch (job.type)
        {
        case JobType::Pull:           return runPull(job);
        case JobType::Push:           return runPush(job);
        case JobType::Commit:         return runCommit(job);
        case JobType::Discard:        return runDiscard(job);
        case JobType::UpdateTemplate: return runUpdateTemplate(job);
        case JobType::Clean:          return runClean(job);
        case JobType::RefreshStatus:  refreshStatus(job.repoPath); return ResultSuccess;
        }
        return ResultFailed;
    }

    int RepositoryJobQueue::runProcessJob(const QString& program, const QStringList& args,
                                          const QString& workingDir, QString* capturedOutput,
                                          int* rawExitCode)
    {
        if (rawExitCode)
            *rawExitCode = -1;
        QProcess proc;
        proc.setProgram(program);
        proc.setArguments(args);
        proc.setWorkingDirectory(workingDir);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        if (!proc.waitForStarted(10000))
        {
            Logging::getLogger().logError("Failed to start: " + program.toStdString());
            return ResultFailed;
        }
        m_currentPid = proc.processId();
        QString buffer;
        while (proc.state() != QProcess::NotRunning)
        {
            proc.waitForFinished(100); // pumps process I/O; returns on timeout or exit
            const QString chunk = QString::fromLocal8Bit(proc.readAll());
            if (!chunk.isEmpty())
            {
                buffer += chunk;
                // stream complete lines into the logger
                int nl;
                while ((nl = buffer.indexOf('\n')) >= 0)
                {
                    Logging::getLogger().logInfo(buffer.left(nl).trimmed().toStdString());
                    if (capturedOutput) *capturedOutput += buffer.left(nl + 1);
                    buffer.remove(0, nl + 1);
                }
            }
            // cancel() has already issued taskkill on m_currentPid; the loop exits when the process dies
        }
        const QString rest = buffer + QString::fromLocal8Bit(proc.readAll());
        if (!rest.trimmed().isEmpty())
        {
            Logging::getLogger().logInfo(rest.trimmed().toStdString());
            if (capturedOutput) *capturedOutput += rest;
        }
        m_currentPid = 0;
        if (m_cancelRequested)
            return ResultCanceled;
        if (proc.exitStatus() != QProcess::NormalExit)
            return ResultFailed;
        if (rawExitCode)
            *rawExitCode = proc.exitCode();
        return proc.exitCode() == 0 ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runPull(const Job& job)
    {
        return Utilities::gitPull(job.repoPath) ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runPush(const Job& job)
    {
        return Utilities::gitPush(job.repoPath) ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runCommit(const Job& job)
    {
        return Utilities::gitCommit(job.repoPath, job.commitMessage) ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runDiscard(const Job& job)
    {
        return Utilities::gitDiscardChanges(job.repoPath) ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runClean(const Job& job)
    {
        const QString buildDir = job.repoPath + "/build";
        Logging::getLogger().logInfo("Cleaning build folder: " + buildDir.toStdString());
        return Utilities::deleteFolderRecursively(buildDir) ? ResultSuccess : ResultFailed;
    }

    int RepositoryJobQueue::runUpdateTemplate(const Job& job)
    {
        const QString tpl = Resources::getCurrentTemplateAbsSourcePath();
        if (!QDir(tpl).exists())
        {
            Logging::getLogger().logError("Template not downloaded — use 'Download template' first");
            return ResultFailed;
        }

        QString before;
        Utilities::readTemplateVersion(job.repoPath + "/CMakeLists.txt", before);

        ProjectSettings settings;
        if (!ProjectExporter::readProjectData(settings, job.repoPath))
            return ResultFailed;

        ProjectExporter::ExportSettings ex;
        ex.copyAllTemplateFiles = false;
        ex.replaceTemplateCmakeFiles = true;
        ex.replaceTemplateCodeFiles = true;
        ex.replaceTemplateVariables = true;
        ex.replaceTemplateCodePlaceholders = true;

        const bool ok = ProjectExporter::exportProject(settings, job.repoPath, ex);
        if (!ok)
            return ResultFailed;
        emit templateUpdated(job.repoPath, before);
        return ResultSuccess;
    }

    void RepositoryJobQueue::refreshStatus(const QString& repoPath)
    {
        RepositoryInfo info;
        info.path = repoPath;
        info.name = QDir(repoPath).dirName();
        info.pathExists = QDir(repoPath).exists();
        if (info.pathExists)
        {
            bool gitOk = false;
            info.hasUncommittedChanges = Utilities::gitIsDirty(repoPath, &gitOk);
            info.isGitRepo = gitOk;
            if (gitOk)
                info.headCommitSubject = Utilities::gitHeadCommitSubject(repoPath);
            QString v;
            const QVector<QString> lines = Utilities::getFileContents(repoPath + "/CMakeLists.txt");
            info.libraryVersion = Utilities::readCmakeVariableString(lines, "LIBRARY_VERSION", v) ? v : "?";
            info.templateVersion = Utilities::readTemplateVersion(repoPath + "/CMakeLists.txt", v) ? v : "?";
        }
        emit statusRefreshed(repoPath, info);
    }

    void RepositoryJobQueue::cancel()
    {
        m_cancelRequested = true;
        const qint64 pid = m_currentPid;
        if (pid != 0)
        {
            Logging::getLogger().logInfo("Canceling: killing process tree " + std::to_string(pid));
            QProcess::startDetached("taskkill", { "/PID", QString::number(pid), "/T", "/F" });
        }
    }

    bool RepositoryJobQueue::isBusy() const
    {
        QMutexLocker lock(&m_mutex);
        return m_busy;
    }

    void RepositoryJobQueue::shutdown(int timeoutMs)
    {
        cancel();
        if (m_thread)
            m_thread->wait(timeoutMs);
    }

    QString RepositoryJobQueue::jobVerb(JobType type)
    {
        switch (type)
        {
        case JobType::Pull:           return "Pulling";
        case JobType::Push:           return "Pushing";
        case JobType::Commit:         return "Committing";
        case JobType::Discard:        return "Discarding changes";
        case JobType::UpdateTemplate: return "Updating template";
        case JobType::Clean:          return "Cleaning";
        case JobType::RefreshStatus:  return "Refreshing status";
        }
        return QString();
    }
}
