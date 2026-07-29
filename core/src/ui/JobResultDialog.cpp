#include "ui/JobResultDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace CLC
{
    JobResultDialog::JobResultDialog(const QString& title, QWidget* parent)
        : QWidget(parent)
    {
        setWindowTitle(title);

        QVBoxLayout* root = new QVBoxLayout(this);

        QScrollArea* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        QWidget* container = new QWidget(scroll);
        m_rowsLayout = new QVBoxLayout(container);
        m_rowsLayout->addStretch(1);   // trailing stretch; rows inserted above it
        scroll->setWidget(container);
        root->addWidget(scroll);

        resize(480, 360);
        hide();
    }

    void JobResultDialog::beginRun()
    {
        for (const Row& row : m_rows)
        {
            m_rowsLayout->removeWidget(row.container);
            row.container->deleteLater();
        }
        m_rows.clear();
    }

    void JobResultDialog::setResult(const QString& repoName, const QString& repoPath,
                                    const QString& resultText, const QString& color)
    {
        auto it = m_rows.find(repoPath);
        if (it == m_rows.end())
        {
            Row row;
            row.container = new QWidget(m_rowsLayout->parentWidget());
            QHBoxLayout* line = new QHBoxLayout(row.container);
            line->setContentsMargins(0, 0, 0, 0);

            QLabel* name = new QLabel(repoName, row.container);
            row.result = new QLabel(row.container);
            QPushButton* showLog = new QPushButton("Show log", row.container);
            connect(showLog, &QPushButton::clicked, this, [this, repoPath]() {
                emit showLogRequested(repoPath);
                });

            line->addWidget(name, 1);
            line->addWidget(row.result);
            line->addWidget(showLog);

            const int insertIndex = m_rowsLayout->count() > 0 ? m_rowsLayout->count() - 1 : 0;
            m_rowsLayout->insertWidget(insertIndex, row.container);
            it = m_rows.insert(repoPath, row);
        }

        it->result->setText(resultText);
        it->result->setStyleSheet("QLabel { color: " + color + "; font-weight: bold; }");
    }

    void JobResultDialog::popup()
    {
        show();
        raise();
        activateWindow();
    }
}
