#include "ui/RepositoryOverviewWidget.h"
#include "ui/RepositoryWidget.h"
#include "Resources.h"
#include "Logging.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace CLC
{
    namespace
    {
        const char* kCardsContainerName = "repoCardsContainer";
    }

    RepositoryOverviewWidget::RepositoryOverviewWidget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* root = new QVBoxLayout(this);

        // Header row: bulk selection controls.
        QHBoxLayout* header = new QHBoxLayout();
        QPushButton* selectAll = new QPushButton("Select all", this);
        QPushButton* deselectAll = new QPushButton("Deselect all", this);
        header->addWidget(selectAll);
        header->addWidget(deselectAll);
        header->addStretch(1);
        root->addLayout(header);

        // Scrollable card list.
        QScrollArea* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        QWidget* container = new QWidget(scroll);
        container->setObjectName(kCardsContainerName);
        QVBoxLayout* cardsLayout = new QVBoxLayout(container);
        cardsLayout->addStretch(1);   // trailing stretch; cards inserted above it
        scroll->setWidget(container);
        root->addWidget(scroll, 1);

        auto setAll = [this](bool enabled)
        {
            for (RepositoryWidget* w : m_cards)
            {
                RepositoryInfo info = w->info();
                info.groupEnabled = enabled;
                w->setInfo(info);                                  // syncs checkbox display
                Resources::setProjectGroupEnabled(info.path, enabled); // persists
            }
        };
        connect(selectAll, &QPushButton::clicked, this, [setAll]() { setAll(true); });
        connect(deselectAll, &QPushButton::clicked, this, [setAll]() { setAll(false); });

        reload();
    }

    void RepositoryOverviewWidget::reload()
    {
        QWidget* container = findChild<QWidget*>(kCardsContainerName);
        if (!container || !container->layout())
            return;
        QVBoxLayout* cardsLayout = qobject_cast<QVBoxLayout*>(container->layout());
        if (!cardsLayout)
            return;

        // Drop existing cards (keep the trailing stretch item).
        for (RepositoryWidget* w : m_cards)
        {
            cardsLayout->removeWidget(w);
            w->deleteLater();
        }
        m_cards.clear();

        const Resources::LoadSaveProjects& projects = Resources::getLoadSaveProjects();
        for (const Resources::ProjectEntry& e : projects.projects)
        {
            RepositoryInfo info;
            info.path = e.path;
            info.name = QDir(e.path).dirName();
            info.groupEnabled = e.groupEnabled;
            info.pathExists = QDir(e.path).exists();
            info.libraryVersion = "?";
            info.templateVersion = "?";

            RepositoryWidget* card = new RepositoryWidget(info, container);
            connect(card, &RepositoryWidget::groupEnabledChanged, this,
                [](const QString& path, bool enabled) { Resources::setProjectGroupEnabled(path, enabled); });
            connect(card, &RepositoryWidget::actionRequested, this, &RepositoryOverviewWidget::actionRequested);
            connect(card, &RepositoryWidget::buildRequested, this, &RepositoryOverviewWidget::buildRequested);
            connect(card, &RepositoryWidget::unitTestRequested, this, &RepositoryOverviewWidget::unitTestRequested);
            connect(card, &RepositoryWidget::terminateRequested, this, &RepositoryOverviewWidget::terminateRequested);
            connect(card, &RepositoryWidget::showTestLogRequested, this, &RepositoryOverviewWidget::showTestLogRequested);

            // Insert above the trailing stretch (always the last layout item).
            const int insertIndex = cardsLayout->count() > 0 ? cardsLayout->count() - 1 : 0;
            cardsLayout->insertWidget(insertIndex, card);
            m_cards.insert(info.path, card);
        }
    }

    QStringList RepositoryOverviewWidget::checkedRepoPaths() const
    {
        QStringList out;
        QStringList skipped;
        for (RepositoryWidget* w : m_cards)
        {
            if (!w->isGroupEnabled())
                continue;
            if (w->info().pathExists)
                out.push_back(w->repositoryPath());
            else
                skipped.push_back(w->repositoryPath());
        }
        if (!skipped.isEmpty())
            Logging::getLogger().logWarning("Skipping missing repositories in group action: "
                + skipped.join(", ").toStdString());
        return out;
    }

    RepositoryInfo RepositoryOverviewWidget::infoFor(const QString& path) const
    {
        if (RepositoryWidget* w = widgetFor(path))
            return w->info();
        return RepositoryInfo();
    }

    void RepositoryOverviewWidget::applyCollisionLock(const QString& path, bool building, bool testing, bool queueActive)
    {
        if (RepositoryWidget* w = widgetFor(path))
            w->applyCollisionLock(building, testing, queueActive);
    }

    void RepositoryOverviewWidget::resetRunFields(const QString& path, bool build)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        RepositoryInfo info = w->info();
        if (build)
        {
            info.buildLog.clear();
            info.buildStatus = RepositoryInfo::BuildStatus::Building;
        }
        else
        {
            info.unitTestLog.clear();
            info.testStatus = RepositoryInfo::TestStatus::Running;
        }
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onStatusRefreshed(const QString& path, const RepositoryInfo& fresh)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        RepositoryInfo merged = fresh;                        // git + versions + existence from refresh
        const RepositoryInfo& old = w->info();
        merged.groupEnabled = old.groupEnabled;               // session/persisted fields preserved:
        merged.buildStatus = old.buildStatus;
        merged.testStatus = old.testStatus;
        merged.unitTestLog = old.unitTestLog;
        merged.buildLog = old.buildLog;                       // keep the just-stored build log for "Show log"
        merged.templateVersionBeforeUpdate = old.templateVersionBeforeUpdate;
        w->setInfo(merged);
    }

    namespace
    {
        RepositoryWidget::State resultToState(int result)
        {
            if (result == RepositoryJobQueue::ResultSuccess)  return RepositoryWidget::Ok;
            if (result == RepositoryJobQueue::ResultCanceled) return RepositoryWidget::Canceled;
            return RepositoryWidget::Fail;
        }
    }

    void RepositoryOverviewWidget::onJobStarted(const QString& path, int jobType)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        const RepositoryJobQueue::JobType type = (RepositoryJobQueue::JobType)jobType;
        if (type == RepositoryJobQueue::JobType::RefreshStatus)
            return;

        w->setButtonState(type, RepositoryWidget::Working, RepositoryJobQueue::jobVerb(type) + "...");
    }

    void RepositoryOverviewWidget::onJobFinished(const QString& path, int jobType, int result)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        const RepositoryJobQueue::JobType type = (RepositoryJobQueue::JobType)jobType;
        if (type == RepositoryJobQueue::JobType::RefreshStatus)
            return;

        w->setButtonState(type, resultToState(result));
    }

    void RepositoryOverviewWidget::onBuildStarted(const QString& path)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        w->setBuildButtonState(RepositoryWidget::Working, "Building...");
        RepositoryInfo info = w->info();
        info.buildStatus = RepositoryInfo::BuildStatus::Building;
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onBuildFinished(const QString& path, int result, const QString& log)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        w->setBuildButtonState(resultToState(result));
        RepositoryInfo info = w->info();
        if (result == RepositoryJobQueue::ResultSuccess)
            info.buildStatus = RepositoryInfo::BuildStatus::Success;
        else if (result == RepositoryJobQueue::ResultCanceled)
            info.buildStatus = RepositoryInfo::BuildStatus::Canceled;
        else
            info.buildStatus = RepositoryInfo::BuildStatus::Error;
        info.buildLog = log;   // enables the build result "Show log"
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onTemplateUpdated(const QString& path, const QString& previousTemplateVersion)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        RepositoryInfo info = w->info();
        info.templateVersionBeforeUpdate = previousTemplateVersion; // implicit refresh brings the new current version
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onUnitTestStarted(const QString& path)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        w->setUnitTestButtonState(RepositoryWidget::Working, "Running unit tests...");
        RepositoryInfo info = w->info();
        info.testStatus = RepositoryInfo::TestStatus::Running;
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onUnitTestFinished(const QString& path, int result, const QString& log,
                                                      const QVector<CLC::UnitTestSuiteResult>& suites)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        w->setUnitTestButtonState(resultToState(result));
        RepositoryInfo info = w->info();
        if (result == RepositoryJobQueue::ResultSuccess)
            info.testStatus = RepositoryInfo::TestStatus::Passed;
        else if (result == RepositoryJobQueue::ResultCanceled)
            info.testStatus = RepositoryInfo::TestStatus::Canceled;
        else
            info.testStatus = RepositoryInfo::TestStatus::Failed;
        info.unitTestLog = log;   // enables the card's "Show log" button via setInfo
        info.unitTestSuites = suites;
        w->setInfo(info);
    }

    void RepositoryOverviewWidget::onUnitTestNoExecutables(const QString& path)
    {
        RepositoryWidget* w = widgetFor(path);
        if (!w) return;
        w->setUnitTestButtonState(RepositoryWidget::Fail);
        RepositoryInfo info = w->info();
        info.testStatus = RepositoryInfo::TestStatus::Error;
        w->setInfo(info);
    }

    RepositoryWidget* RepositoryOverviewWidget::widgetFor(const QString& path) const
    {
        return m_cards.value(path, nullptr);
    }
}
