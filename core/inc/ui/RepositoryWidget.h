#pragma once

#include "CmakeLibraryCreator_base.h"
#include "RepositoryInfo.h"
#include "RepositoryJobQueue.h"
#include <QFrame>
#include <QMap>

class QCheckBox; class QLabel; class QPushButton; class QTimer;

namespace CLC
{
    class RepositoryWidget : public QFrame
    {
        Q_OBJECT
    public:
        // Per-button lifecycle state shown as a small label under each threaded button.
        enum State { Idle, Working, Ok, Fail, Canceled };

        explicit RepositoryWidget(const RepositoryInfo& info, QWidget* parent = nullptr);

        const QString& repositoryPath() const { return m_info.path; }
        const RepositoryInfo& info() const { return m_info; }
        void setInfo(const RepositoryInfo& info);   // full replace + label refresh
        bool isGroupEnabled() const;

        // Extend button disabling to the sibling buttons that would collide with an
        // in-progress operation on THIS repo, without fighting the per-button Working
        // state. Never affects other cards.
        void applyCollisionLock(bool building, bool testing, bool queueActive);

        // Drive the per-button working/result label + color. Queue actions are keyed by JobType;
        // the off-queue build + unittest buttons have their own entry points.
        void setButtonState(RepositoryJobQueue::JobType type, State state, const QString& runningTooltip = QString());
        void setBuildButtonState(State state, const QString& runningTooltip = QString());
        void setUnitTestButtonState(State state, const QString& runningTooltip = QString());

    signals:
        void groupEnabledChanged(const QString& path, bool enabled);
        void actionRequested(const QString& path, CLC::RepositoryJobQueue::JobType type);
        void buildRequested(const QString& path);      // off-queue: runs via BuildRunner
        void unitTestRequested(const QString& path);   // off-queue: runs via UnitTestRunner
        void terminateRequested(const QString& path);  // taskkill the current build or unittest
        void showTestLogRequested(const QString& path);

    private:
        // Local button identity; UnitTest has no matching JobType (runs off-queue).
        enum class ActionKey { UpdateTemplate, Pull, Push, Commit, Discard, Build, Clean, UnitTest };

        struct ActionButton
        {
            QPushButton* button = nullptr;
            QLabel* label = nullptr;
            QString originalTooltip;
            QTimer* timer = nullptr;
            bool working = false;
        };

        void updateLabels();
        void refreshButtonEnabled();                // enable/disable respecting collision lock + per-button working
        bool isKeyCollisionLocked(ActionKey key) const;
        // Static availability based on repo capabilities (git present, remote configured, ...).
        // Independent of runtime collisions. When false, reasonOut is filled with a user-facing reason.
        bool isKeyStaticallyAvailable(ActionKey key, QString* reasonOut = nullptr) const;
        void applyState(ActionKey key, State state, const QString& runningTooltip);
        QWidget* makeActionButton(const QString& text, const QString& iconPath, const QString& tooltip,
                                  ActionKey key, RepositoryJobQueue::JobType type, QWidget* parent);
        static bool mapJobType(RepositoryJobQueue::JobType type, ActionKey& keyOut);

        RepositoryInfo m_info;
        // Per-repo collision state (this card only); drives sibling-button disabling.
        bool m_lockBuilding = false;
        bool m_lockTesting = false;
        bool m_lockQueueActive = false;
        QCheckBox* m_groupCheckBox;
        QLabel* m_nameLabel;
        QLabel* m_libVersionLabel;
        QLabel* m_templateVersionLabel;   // shows "1.7.0 (was 1.6.1)" after an update
        QLabel* m_dirtyLabel;
        QLabel* m_headCommitLabel;
        QLabel* m_buildStatusLabel;
        QLabel* m_testStatusLabel;
        QPushButton* m_showLogButton;
        QPushButton* m_terminateButton = nullptr;   // enabled only while Build/UnitTest is Working
        QMap<ActionKey, ActionButton> m_actions;   // all threaded buttons (queue actions + unittest)
    };
}
