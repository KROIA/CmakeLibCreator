#include "ui/RepositoryWidget.h"
#include "Utilities.h"

#include <QCheckBox>
#include <QFontMetrics>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace CLC
{
    namespace
    {
        // Small helper: colored plain text for a status label.
        void setColoredText(QLabel* label, const QString& text, const QString& color, bool bold = false)
        {
            label->setText(text);
            QString sheet;
            if (!color.isEmpty())
                sheet += "color: " + color + ";";
            if (bold)
                sheet += "font-weight: bold;";
            label->setStyleSheet(sheet);
        }

        const char* kWorkingButtonStyle = "QPushButton { background-color: #2f6fb0; color: white; }";
        const int kLabelClearMs = 3000;

        QString workingWord(int key) // key is ActionKey underlying value
        {
            switch (key)
            {
            case 0: return "update...";  // UpdateTemplate
            case 1: return "pull...";
            case 2: return "push...";
            case 3: return "commit...";
            case 4: return "discard...";
            case 5: return "build...";
            case 6: return "clean...";
            case 7: return "test...";    // UnitTest
            }
            return QString();
        }
    }

    RepositoryWidget::RepositoryWidget(const RepositoryInfo& info, QWidget* parent)
        : QFrame(parent)
        , m_info(info)
        , m_groupCheckBox(new QCheckBox(this))
        , m_nameLabel(new QLabel(this))
        , m_libVersionLabel(new QLabel(this))
        , m_templateVersionLabel(new QLabel(this))
        , m_dirtyLabel(new QLabel(this))
        , m_headCommitLabel(new QLabel(this))
        , m_buildStatusLabel(new QLabel(this))
        , m_testStatusLabel(new QLabel(this))
        , m_showLogButton(new QPushButton("Show log", this))
    {
        // A real QFrame panel guarantees a visible border separating cards regardless of the app stylesheet.
        setFrameShape(QFrame::StyledPanel);
        setFrameShadow(QFrame::Raised);

        QVBoxLayout* root = new QVBoxLayout(this);

        // Row 0: checkbox, name, versions.
        m_groupCheckBox->setToolTip("Include this repository in group actions");
        {
            QFont f = m_nameLabel->font();
            f.setBold(true);
            f.setPointSize(f.pointSize() + 2);
            m_nameLabel->setFont(f);
        }
        QHBoxLayout* row0 = new QHBoxLayout();
        row0->addWidget(m_groupCheckBox);
        row0->addWidget(m_nameLabel);
        row0->addWidget(m_libVersionLabel);
        row0->addWidget(m_templateVersionLabel);
        row0->addStretch(1);
        root->addLayout(row0);

        // Row 1: status label pairs (2 pairs per grid row).
        QGridLayout* row1 = new QGridLayout();
        row1->addWidget(new QLabel("Uncommitted changes:", this), 0, 0);
        row1->addWidget(m_dirtyLabel, 0, 1);
        row1->addWidget(new QLabel("HEAD:", this), 0, 2);
        row1->addWidget(m_headCommitLabel, 0, 3);
        row1->addWidget(new QLabel("Build:", this), 1, 0);
        row1->addWidget(m_buildStatusLabel, 1, 1);
        row1->addWidget(new QLabel("Unittest:", this), 1, 2);
        {
            QHBoxLayout* testCell = new QHBoxLayout();
            testCell->addWidget(m_testStatusLabel);
            m_showLogButton->setToolTip("Show the last captured unittest log");
            testCell->addWidget(m_showLogButton);
            testCell->addStretch(1);
            row1->addLayout(testCell, 1, 3);
        }
        row1->setColumnStretch(3, 1);
        root->addLayout(row1);

        // Row 2: three captioned button clusters. Each threaded button is a button-on-top /
        // status-label-under vertical pair so the label clearly belongs to its button.
        QHBoxLayout* row2 = new QHBoxLayout();

        QGroupBox* generalBox = new QGroupBox("General", this);
        {
            QHBoxLayout* h = new QHBoxLayout(generalBox);
            QPushButton* openFolder = new QPushButton("Open folder", generalBox);
            openFolder->setIcon(QIcon(":/icons/folder-open.png"));
            openFolder->setToolTip("Open the repository folder in the file explorer");
            connect(openFolder, &QPushButton::clicked, this, [this]()
            {
                Utilities::openFolderInExplorer(m_info.path);   // always works, never queued
            });
            // "Open folder" is instant -> aligned to the button-top of the paired buttons via top alignment.
            QVBoxLayout* openPair = new QVBoxLayout();
            openPair->addWidget(openFolder);
            openPair->addStretch(1);
            h->addLayout(openPair);
            h->addWidget(makeActionButton("Update template", ":/icons/save.png",
                "Re-applies the latest downloaded library template",
                ActionKey::UpdateTemplate, RepositoryJobQueue::JobType::UpdateTemplate, generalBox));
        }
        row2->addWidget(generalBox);

        QGroupBox* gitBox = new QGroupBox("Git", this);
        {
            QHBoxLayout* h = new QHBoxLayout(gitBox);
            h->addWidget(makeActionButton("Pull", ":/icons/download.png", "git pull --ff-only",
                ActionKey::Pull, RepositoryJobQueue::JobType::Pull, gitBox));
            h->addWidget(makeActionButton("Push", ":/icons/upload.png", "git push to the upstream branch",
                ActionKey::Push, RepositoryJobQueue::JobType::Push, gitBox));
            h->addWidget(makeActionButton("Commit", ":/icons/accept.png", "Commit all changes with a message",
                ActionKey::Commit, RepositoryJobQueue::JobType::Commit, gitBox));
            h->addWidget(makeActionButton("Discard", "", "Discards all changes (git reset --hard + git clean -fd)",
                ActionKey::Discard, RepositoryJobQueue::JobType::Discard, gitBox));
        }
        row2->addWidget(gitBox);

        QGroupBox* buildBox = new QGroupBox("Build", this);
        {
            QHBoxLayout* h = new QHBoxLayout(buildBox);
            // Build runs off the sequential queue via BuildRunner (parallel pool). The JobType arg
            // is a placeholder; the clicked handler is rewired below to emit buildRequested.
            QWidget* buildPair = makeActionButton("Build", ":/icons/hammer.png", "Runs build.bat (x64-Debug + x64-Release)",
                ActionKey::Build, RepositoryJobQueue::JobType::RefreshStatus, buildBox);
            QPushButton* buildBtn = m_actions[ActionKey::Build].button;
            disconnect(buildBtn, &QPushButton::clicked, this, nullptr);
            connect(buildBtn, &QPushButton::clicked, this, [this]()
            {
                emit buildRequested(m_info.path);
            });
            h->addWidget(buildPair);
            h->addWidget(makeActionButton("Clean", "", "Deletes the build folder",
                ActionKey::Clean, RepositoryJobQueue::JobType::Clean, buildBox));
            // Unittest runs off the sequential queue (own runner). It keeps its own ActionKey so it
            // stays clickable/independent while queue jobs for other repos run.
            QWidget* testPair = makeActionButton("Unittest", ":/icons/accept.png",
                "Runs the repository's unit tests (build/Release/*.exe)",
                ActionKey::UnitTest, RepositoryJobQueue::JobType::RefreshStatus, buildBox);
            // Rewire: unittest emits its dedicated off-queue signal, not actionRequested.
            QPushButton* testBtn = m_actions[ActionKey::UnitTest].button;
            disconnect(testBtn, &QPushButton::clicked, this, nullptr);
            connect(testBtn, &QPushButton::clicked, this, [this]()
            {
                emit unitTestRequested(m_info.path);
            });
            h->addWidget(testPair);
        }
        row2->addWidget(buildBox);

        root->addLayout(row2);

        // In-card signal wiring.
        connect(m_groupCheckBox, &QCheckBox::toggled, this, [this](bool checked)
        {
            emit groupEnabledChanged(m_info.path, checked);
        });
        connect(m_showLogButton, &QPushButton::clicked, this, [this]()
        {
            emit showTestLogRequested(m_info.path);
        });

        setInfo(m_info);
    }

    QWidget* RepositoryWidget::makeActionButton(const QString& text, const QString& iconPath, const QString& tooltip,
                                                ActionKey key, RepositoryJobQueue::JobType type, QWidget* parent)
    {
        QWidget* pair = new QWidget(parent);
        QVBoxLayout* v = new QVBoxLayout(pair);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(1);

        QPushButton* b = new QPushButton(text, pair);
        if (!iconPath.isEmpty())
            b->setIcon(QIcon(iconPath));
        b->setToolTip(tooltip);

        QLabel* status = new QLabel(pair);
        status->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        {
            QFont f = status->font();
            f.setPointSize(std::max(1, f.pointSize() - 2));
            status->setFont(f);
        }
        // Clip the status text to the button width so it never widens the pair.
        status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

        v->addWidget(b);
        v->addWidget(status);

        QTimer* timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(kLabelClearMs);
        connect(timer, &QTimer::timeout, this, [this, key]()
        {
            auto it = m_actions.find(key);
            if (it != m_actions.end())
                it->label->clear();
        });

        // Queue actions emit actionRequested; the unittest button is rewired by the caller.
        connect(b, &QPushButton::clicked, this, [this, type]()
        {
            emit actionRequested(m_info.path, type);
        });

        ActionButton entry;
        entry.button = b;
        entry.label = status;
        entry.originalTooltip = tooltip;
        entry.timer = timer;
        m_actions.insert(key, entry);
        return pair;
    }

    bool RepositoryWidget::mapJobType(RepositoryJobQueue::JobType type, ActionKey& keyOut)
    {
        switch (type)
        {
        case RepositoryJobQueue::JobType::UpdateTemplate: keyOut = ActionKey::UpdateTemplate; return true;
        case RepositoryJobQueue::JobType::Pull:           keyOut = ActionKey::Pull;           return true;
        case RepositoryJobQueue::JobType::Push:           keyOut = ActionKey::Push;           return true;
        case RepositoryJobQueue::JobType::Commit:         keyOut = ActionKey::Commit;         return true;
        case RepositoryJobQueue::JobType::Discard:        keyOut = ActionKey::Discard;        return true;
        case RepositoryJobQueue::JobType::Clean:          keyOut = ActionKey::Clean;          return true;
        case RepositoryJobQueue::JobType::RefreshStatus:  return false;
        }
        return false;
    }

    void RepositoryWidget::setButtonState(RepositoryJobQueue::JobType type, State state, const QString& runningTooltip)
    {
        ActionKey key;
        if (mapJobType(type, key))
            applyState(key, state, runningTooltip);
    }

    void RepositoryWidget::setBuildButtonState(State state, const QString& runningTooltip)
    {
        applyState(ActionKey::Build, state, runningTooltip);
    }

    void RepositoryWidget::setUnitTestButtonState(State state, const QString& runningTooltip)
    {
        applyState(ActionKey::UnitTest, state, runningTooltip);
    }

    void RepositoryWidget::applyState(ActionKey key, State state, const QString& runningTooltip)
    {
        auto it = m_actions.find(key);
        if (it == m_actions.end())
            return;
        ActionButton& a = *it;
        const int keyIndex = static_cast<int>(key);

        switch (state)
        {
        case Idle:
            a.working = false;
            a.timer->stop();
            a.label->clear();
            a.button->setStyleSheet(QString());
            a.button->setToolTip(a.originalTooltip);
            break;
        case Working:
            a.working = true;
            a.timer->stop();
            setColoredText(a.label, workingWord(keyIndex), "#2f6fb0");
            a.button->setStyleSheet(kWorkingButtonStyle);
            a.button->setToolTip(runningTooltip.isEmpty() ? a.originalTooltip : runningTooltip);
            break;
        case Ok:
            a.working = false;
            setColoredText(a.label, "ok", "green", true);
            a.button->setStyleSheet(QString());
            a.button->setToolTip(a.originalTooltip);
            a.timer->start();
            break;
        case Fail:
            a.working = false;
            setColoredText(a.label, "fail", "red", true);
            a.button->setStyleSheet(QString());
            a.button->setToolTip(a.originalTooltip);
            a.timer->start();
            break;
        case Canceled:
            a.working = false;
            setColoredText(a.label, "cancel", "orange", true);
            a.button->setStyleSheet(QString());
            a.button->setToolTip(a.originalTooltip);
            a.timer->start();
            break;
        }
        refreshButtonEnabled();
    }

    bool RepositoryWidget::isGroupEnabled() const
    {
        return m_groupCheckBox->isChecked();
    }

    void RepositoryWidget::setInfo(const RepositoryInfo& info)
    {
        m_info = info;
        {
            QSignalBlocker block(m_groupCheckBox);
            m_groupCheckBox->setChecked(m_info.groupEnabled);
        }
        updateLabels();
    }

    void RepositoryWidget::applyCollisionLock(bool building, bool testing, bool queueActive)
    {
        m_lockBuilding = building;
        m_lockTesting = testing;
        m_lockQueueActive = queueActive;
        refreshButtonEnabled();
    }

    bool RepositoryWidget::isKeyCollisionLocked(ActionKey key) const
    {
        // An active queue job on this repo blocks any new mutating op on this repo.
        if (m_lockQueueActive)
            return true;
        switch (key)
        {
        case ActionKey::Build:          return m_lockBuilding || m_lockTesting;
        case ActionKey::Clean:          return m_lockBuilding || m_lockTesting;
        case ActionKey::UnitTest:       return m_lockBuilding || m_lockTesting;
        case ActionKey::UpdateTemplate: return m_lockBuilding;
        default:                        return false;   // git ops collide only with a queue job
        }
    }

    void RepositoryWidget::refreshButtonEnabled()
    {
        for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
        {
            // A Working button always stays disabled; collision extends disabling to siblings.
            const bool enabled = m_info.pathExists && !it->working && !isKeyCollisionLocked(it.key());
            it->button->setEnabled(enabled);
        }
    }

    void RepositoryWidget::updateLabels()
    {
        // Name (+ red "(not found)" when the path is missing).
        if (m_info.pathExists)
            m_nameLabel->setText(m_info.name);
        else
            m_nameLabel->setText(m_info.name + "  <span style=\"color:red;\">(not found)</span>");
        m_nameLabel->setTextFormat(Qt::RichText);

        m_libVersionLabel->setText("Lib: " + m_info.libraryVersion);

        QString tpl = "Template: " + m_info.templateVersion;
        if (!m_info.templateVersionBeforeUpdate.isEmpty()
            && m_info.templateVersionBeforeUpdate != m_info.templateVersion)
            tpl += " (was " + m_info.templateVersionBeforeUpdate + ")";
        m_templateVersionLabel->setText(tpl);

        // Uncommitted changes: No = green bold, Yes = orange bold, unknown = gray.
        if (!m_info.isGitRepo)
            setColoredText(m_dirtyLabel, "?", "gray");
        else if (m_info.hasUncommittedChanges)
            setColoredText(m_dirtyLabel, "Yes", "orange", true);
        else
            setColoredText(m_dirtyLabel, "No", "green", true);

        // HEAD subject (elided, full text as tooltip).
        const QString subject = m_info.headCommitSubject.isEmpty() ? QString::fromUtf8("\xE2\x80\x94") : m_info.headCommitSubject;
        const QString elided = m_headCommitLabel->fontMetrics().elidedText(subject, Qt::ElideRight, 240);
        m_headCommitLabel->setText(elided);
        m_headCommitLabel->setToolTip(m_info.headCommitSubject);

        switch (m_info.buildStatus)
        {
        case RepositoryInfo::BuildStatus::NotBuilt: setColoredText(m_buildStatusLabel, "not built", "gray"); break;
        case RepositoryInfo::BuildStatus::Building: setColoredText(m_buildStatusLabel, "building...", "blue"); break;
        case RepositoryInfo::BuildStatus::Error:    setColoredText(m_buildStatusLabel, "build error", "red"); break;
        case RepositoryInfo::BuildStatus::Success:  setColoredText(m_buildStatusLabel, "build ok", "green"); break;
        case RepositoryInfo::BuildStatus::Canceled: setColoredText(m_buildStatusLabel, "canceled", "orange"); break;
        }

        switch (m_info.testStatus)
        {
        case RepositoryInfo::TestStatus::NotRun:   setColoredText(m_testStatusLabel, "not run", "gray"); break;
        case RepositoryInfo::TestStatus::Running:  setColoredText(m_testStatusLabel, "running...", "blue"); break;
        case RepositoryInfo::TestStatus::Passed:   setColoredText(m_testStatusLabel, "passed", "green"); break;
        case RepositoryInfo::TestStatus::Failed:   setColoredText(m_testStatusLabel, "failed", "red"); break;
        case RepositoryInfo::TestStatus::Error:    setColoredText(m_testStatusLabel, "error", "red"); break;
        case RepositoryInfo::TestStatus::Canceled: setColoredText(m_testStatusLabel, "canceled", "orange"); break;
        }

        m_showLogButton->setEnabled(!m_info.unitTestLog.isEmpty());

        // Keep action-button enable state consistent; must not stomp a per-button Working state.
        refreshButtonEnabled();
    }
}
