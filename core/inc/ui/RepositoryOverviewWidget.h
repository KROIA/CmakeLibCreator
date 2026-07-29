#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RepositoryInfo.h"
#include "RepositoryJobQueue.h"
#include <QWidget>
#include <QMap>

namespace CLC
{
    class RepositoryWidget;

    class RepositoryOverviewWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit RepositoryOverviewWidget(QWidget* parent = nullptr);

        void reload();                                  // rebuild cards from Resources
        QStringList checkedRepoPaths() const;           // group-enabled + existing paths
        RepositoryInfo infoFor(const QString& path) const;
        // Push per-repo collision state to a single card (never touches other cards).
        void applyCollisionLock(const QString& path, bool building, bool testing, bool queueActive);
        // Reset per-repo fields at the start of a fresh run: clears the log and
        // sets status back to Building (build) / Running (unittest).
        void resetRunFields(const QString& path, bool build);

    public slots:
        void onStatusRefreshed(const QString& path, const CLC::RepositoryInfo& info);
        void onJobStarted(const QString& path, int jobType);
        void onJobFinished(const QString& path, int jobType, int result);
        void onTemplateUpdated(const QString& path, const QString& previousTemplateVersion);
        // Driven by BuildRunner (off-queue parallel pool), per repo.
        void onBuildStarted(const QString& path);
        void onBuildFinished(const QString& path, int result, const QString& log);
        // Driven by UnitTestRunner (off-queue), independent of the queue's busy state.
        void onUnitTestStarted(const QString& path);
        void onUnitTestFinished(const QString& path, int result, const QString& log,
                                const QVector<CLC::UnitTestSuiteResult>& suites);
        void onUnitTestNoExecutables(const QString& path);

    signals:
        void actionRequested(const QString& path, CLC::RepositoryJobQueue::JobType type);
        void buildRequested(const QString& path);
        void unitTestRequested(const QString& path);
        void terminateRequested(const QString& path);
        void showTestLogRequested(const QString& path);

    private:
        RepositoryWidget* widgetFor(const QString& path) const;

        QMap<QString, RepositoryWidget*> m_cards;   // key: repo path
    };
}
