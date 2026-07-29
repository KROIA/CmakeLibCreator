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
#include <QSet>
#include <QRegularExpression>
#include <algorithm>
#include <cstdlib>
#include <string>

namespace CLC
{
    namespace
    {
        // Lowercase + strip separators + strip trailing "unittest(s)"/"test(s)" tokens.
        // Lets folders like "MyLib" match "MyLib_UnitTest.exe" or "MyLibTests.exe".
        QString normalizeToken(QString s)
        {
            s = s.toLower();
            s.remove(QRegularExpression("[_\\-\\s\\.]"));
            static const QStringList suffixes = { "unittests", "unittest", "tests", "test" };
            bool changed = true;
            while (changed)
            {
                changed = false;
                for (const QString& sf : suffixes)
                {
                    if (s.size() > sf.size() && s.endsWith(sf))
                    {
                        s.chop(sf.size());
                        changed = true;
                        break;
                    }
                }
            }
            return s;
        }

        // Best-match exe for a unittest folder. Order: (1) exact case-insensitive
        // "<suite>.exe"; (2) exes whose normalized stem equals normalized suite;
        // (3) substring match either direction on the normalized tokens, tie-broken
        // by smallest length delta. Returns empty basename if nothing matches.
        QString findMatchingExe(const QString& suite, const QStringList& releaseExes)
        {
            for (const QString& exe : releaseExes)
                if (QFileInfo(exe).completeBaseName().compare(suite, Qt::CaseInsensitive) == 0)
                    return exe;

            const QString nSuite = normalizeToken(suite);
            if (nSuite.isEmpty())
                return QString();

            QString exact;
            QVector<QString> subs;
            for (const QString& exe : releaseExes)
            {
                const QString stem = QFileInfo(exe).completeBaseName();
                const QString nStem = normalizeToken(stem);
                if (nStem.isEmpty())
                    continue;
                if (nStem == nSuite)
                {
                    exact = exe;
                    break;
                }
                if (nStem.contains(nSuite) || nSuite.contains(nStem))
                    subs.push_back(exe);
            }
            if (!exact.isEmpty())
                return exact;
            if (subs.isEmpty())
                return QString();

            std::sort(subs.begin(), subs.end(), [&](const QString& a, const QString& b) {
                const int da = std::abs(normalizeToken(QFileInfo(a).completeBaseName()).size() - nSuite.size());
                const int db = std::abs(normalizeToken(QFileInfo(b).completeBaseName()).size() - nSuite.size());
                if (da != db) return da < db;
                return a.size() < b.size();
            });
            return subs.first();
        }
    }

    UnitTestRunner::UnitTestRunner(QObject* parent)
        : QObject(parent)
    {
        qRegisterMetaType<QVector<CLC::UnitTestSuiteResult>>("QVector<CLC::UnitTestSuiteResult>");
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

        // Enumerate every exe in Release once; fuzzy-match each suite folder against it.
        const QStringList allExes = QDir(releaseDir).entryList(QStringList() << "*.exe", QDir::Files);

        QVector<UnitTestSuiteResult> matched;         // suite folder -> matched exe (if any)
        QVector<UnitTestSuiteResult> unmatched;       // suite folder with no matching exe
        QSet<QString> usedExes;                       // don't re-run the same exe for two folders
        for (const QString& suite : suites)
        {
            const QString exe = findMatchingExe(suite, allExes);
            if (exe.isEmpty() || usedExes.contains(exe))
            {
                UnitTestSuiteResult r;
                r.suiteName = suite;
                r.result = RepositoryJobQueue::ResultFailed;
                r.log = exe.isEmpty()
                    ? QString("No matching executable in build/Release for folder '%1'.\n").arg(suite)
                    : QString("Executable '%1' already used for another suite; skipped.\n").arg(exe);
                unmatched.push_back(r);
                continue;
            }
            usedExes.insert(exe);
            UnitTestSuiteResult r;
            r.suiteName = suite;
            r.exeName = exe;
            matched.push_back(r);
        }

        if (matched.isEmpty())
        {
            emit noExecutables(repoPath);
            return;
        }

        QString fullLog;
        bool anyFailure = false;
        bool canceled = false;

        for (UnitTestSuiteResult& sr : matched)
        {
            const QString exePath = releaseDir + "/" + sr.exeName;
            QString out;
            int raw = -1;
            const int r = runProcess(exePath, releaseDir, &out, &raw, repoPath);
            sr.log = out;
            fullLog += "===== " + sr.suiteName + " (" + sr.exeName + ") =====\n" + out + "\n";

            if (r == RepositoryJobQueue::ResultCanceled)
            {
                sr.result = RepositoryJobQueue::ResultCanceled;
                canceled = true;
                break;
            }
            // Inverted exit-code convention (KROIA UnitTest template):
            // NormalExit && exitCode != 0 == PASS; code 0 or crash == FAIL.
            if (raw <= 0)
            {
                sr.result = RepositoryJobQueue::ResultFailed;
                anyFailure = true;
            }
            else
            {
                sr.result = RepositoryJobQueue::ResultSuccess;
            }
        }

        // Fold unmatched folders in as failed suites so the UI can surface them.
        for (const UnitTestSuiteResult& u : unmatched)
        {
            matched.push_back(u);
            fullLog += "===== " + u.suiteName + " (no exe) =====\n" + u.log + "\n";
            anyFailure = true;
        }

        const int result = canceled
            ? RepositoryJobQueue::ResultCanceled
            : (anyFailure ? RepositoryJobQueue::ResultFailed : RepositoryJobQueue::ResultSuccess);
        {
            QMutexLocker lock(&m_mutex);
            m_canceledRepos.remove(repoPath);   // consumed; next run starts clean
        }
        emit finished(repoPath, result, fullLog, matched);
    }

    int UnitTestRunner::runProcess(const QString& program, const QString& workingDir,
                                   QString* capturedOutput, int* rawExitCode, const QString& repoPath)
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
        {
            QMutexLocker lock(&m_mutex);
            m_pids.insert(repoPath, proc.processId());
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
        bool wasCanceled = false;
        {
            QMutexLocker lock(&m_mutex);
            m_pids.remove(repoPath);
            wasCanceled = m_canceledRepos.contains(repoPath);
        }
        if (wasCanceled)
            return RepositoryJobQueue::ResultCanceled;
        if (proc.exitStatus() != QProcess::NormalExit)
            return RepositoryJobQueue::ResultFailed;
        if (rawExitCode)
            *rawExitCode = proc.exitCode();
        return proc.exitCode() == 0 ? RepositoryJobQueue::ResultSuccess : RepositoryJobQueue::ResultFailed;
    }

    void UnitTestRunner::cancelAll()
    {
        QList<QString> repos;
        QList<qint64> pids;
        {
            QMutexLocker lock(&m_mutex);
            repos = m_active.values();
            for (const QString& r : repos)
                m_canceledRepos.insert(r);
            pids = m_pids.values();
        }
        for (const qint64 pid : pids)
        {
            if (pid == 0) continue;
            Logging::getLogger().logInfo("Canceling unittest: killing process tree " + std::to_string(pid));
            QProcess::startDetached("taskkill", { "/PID", QString::number(pid), "/T", "/F" });
        }
    }

    void UnitTestRunner::cancel(const QString& repoPath)
    {
        qint64 pid = 0;
        {
            QMutexLocker lock(&m_mutex);
            if (!m_active.contains(repoPath))
                return;
            m_canceledRepos.insert(repoPath);       // executeTests() breaks the suite loop on next iter
            pid = m_pids.value(repoPath, 0);
        }
        if (pid != 0)
        {
            Logging::getLogger().logInfo("Canceling unittest: killing process tree " + std::to_string(pid));
            QProcess::startDetached("taskkill", { "/PID", QString::number(pid), "/T", "/F" });
        }
    }
}
