#include "UnitTestRunner.h"
#include "RepositoryJobQueue.h"
#include "Logging.h"

#include <QThread>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QList>
#include <QVector>
#include <string>

namespace CLC
{
    UnitTestRunner::UnitTestRunner(QObject* parent)
        : QObject(parent)
    {
    }

    UnitTestRunner::~UnitTestRunner()
    {
        // Best-effort join so in-flight test processes drain before teardown.
        QList<QThread*> threads;
        {
            QMutexLocker lock(&m_mutex);
            threads = m_threads.values();
        }
        for (QThread* t : threads)
            if (t) t->wait(5000);
    }

    void UnitTestRunner::run(const QString& repoPath)
    {
        {
            QMutexLocker lock(&m_mutex);
            if (m_active.contains(repoPath))
                return;                       // refuse a second run for the same repo
            m_active.insert(repoPath);
        }

        QThread* thread = new QThread();
        {
            QMutexLocker lock(&m_mutex);
            m_threads.insert(repoPath, thread);
        }

        // No context object: the functor runs as a direct connection on the emitting
        // (worker) thread, so executeTests() runs off the GUI thread. Signals reach GUI
        // slots via queued AutoConnection because this runner lives on the GUI thread.
        QObject::connect(thread, &QThread::started, [this, repoPath, thread]() {
            executeTests(repoPath);
            thread->exit();
        });
        QObject::connect(thread, &QThread::finished, this, [this, repoPath, thread]() {
            {
                QMutexLocker lock(&m_mutex);
                m_active.remove(repoPath);
                m_threads.remove(repoPath);
            }
            thread->deleteLater();
        });
        thread->start();
    }

    bool UnitTestRunner::isRunning(const QString& repoPath) const
    {
        QMutexLocker lock(&m_mutex);
        return m_active.contains(repoPath);
    }

    void UnitTestRunner::executeTests(const QString& repoPath)
    {
        emit started(repoPath);

        // Resolve the test dir case-insensitively (template spells it "unitTests").
        QString testDirName;
        const QStringList dirs = QDir(repoPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& d : dirs)
        {
            if (d.compare("unittests", Qt::CaseInsensitive) == 0)
            {
                testDirName = d;
                break;
            }
        }
        if (testDirName.isEmpty())
        {
            emit noExecutables(repoPath);
            return;
        }

        const QString testDir = repoPath + "/" + testDirName;
        const QStringList suites = QDir(testDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        const QString releaseDir = repoPath + "/build/Release";

        QVector<QString> existing;   // suite names whose Release exe is present
        for (const QString& suite : suites)
            if (QFileInfo::exists(releaseDir + "/" + suite + ".exe"))
                existing.push_back(suite);

        if (existing.isEmpty())
        {
            emit noExecutables(repoPath);   // built elsewhere is fine, but nothing to run here
            return;
        }

        QString fullLog;
        bool anyFailure = false;
        bool canceled = false;

        for (const QString& suite : existing)
        {
            const QString exe = releaseDir + "/" + suite + ".exe";
            QString out;
            int raw = -1;
            const int r = runProcess(exe, releaseDir, &out, &raw);
            fullLog += "===== " + suite + " =====\n" + out + "\n";

            if (r == RepositoryJobQueue::ResultCanceled)
            {
                canceled = true;
                break;
            }
            // Inverted exit-code convention (KROIA UnitTest template):
            // NormalExit && exitCode != 0 == PASS; code 0 or crash == FAIL.
            if (raw <= 0)
                anyFailure = true;
        }

        const int result = canceled
            ? RepositoryJobQueue::ResultCanceled
            : (anyFailure ? RepositoryJobQueue::ResultFailed : RepositoryJobQueue::ResultSuccess);
        emit finished(repoPath, result, fullLog);
    }

    int UnitTestRunner::runProcess(const QString& program, const QString& workingDir,
                                   QString* capturedOutput, int* rawExitCode)
    {
        if (rawExitCode)
            *rawExitCode = -1;
        QProcess proc;
        proc.setProgram(program);
        proc.setWorkingDirectory(workingDir);
        proc.setProcessChannelMode(QProcess::MergedChannels);
        proc.start();
        if (!proc.waitForStarted(10000))
        {
            Logging::getLogger().logError("Failed to start: " + program.toStdString());
            return RepositoryJobQueue::ResultFailed;
        }
        QString buffer;
        while (proc.state() != QProcess::NotRunning)
        {
            proc.waitForFinished(100); // pumps process I/O; returns on timeout or exit
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
        }
        const QString rest = buffer + QString::fromLocal8Bit(proc.readAll());
        if (!rest.trimmed().isEmpty())
        {
            Logging::getLogger().logInfo(rest.trimmed().toStdString());
            if (capturedOutput) *capturedOutput += rest;
        }
        if (proc.exitStatus() != QProcess::NormalExit)
            return RepositoryJobQueue::ResultFailed;
        if (rawExitCode)
            *rawExitCode = proc.exitCode();
        return proc.exitCode() == 0 ? RepositoryJobQueue::ResultSuccess : RepositoryJobQueue::ResultFailed;
    }
}
