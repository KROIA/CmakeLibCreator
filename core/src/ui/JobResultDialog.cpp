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

        resize(560, 400);
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
                                    const QString& resultText, const QString& color,
                                    const QVector<SubItem>& subItems)
    {
        auto it = m_rows.find(repoPath);
        if (it == m_rows.end())
        {
            Row row;
            row.container = new QWidget(m_rowsLayout->parentWidget());
            QVBoxLayout* outer = new QVBoxLayout(row.container);
            outer->setContentsMargins(0, 0, 0, 0);
            outer->setSpacing(2);

            QWidget* head = new QWidget(row.container);
            QHBoxLayout* line = new QHBoxLayout(head);
            line->setContentsMargins(0, 0, 0, 0);

            QLabel* name = new QLabel(repoName, head);
            row.result = new QLabel(head);
            QPushButton* showLog = new QPushButton("Show log", head);
            connect(showLog, &QPushButton::clicked, this, [this, repoPath]() {
                emit showLogRequested(repoPath, QString());
                });

            line->addWidget(name, 1);
            line->addWidget(row.result);
            line->addWidget(showLog);
            outer->addWidget(head);

            row.subContainer = new QWidget(row.container);
            QVBoxLayout* subLayout = new QVBoxLayout(row.subContainer);
            subLayout->setContentsMargins(20, 0, 0, 0);
            subLayout->setSpacing(1);
            outer->addWidget(row.subContainer);

            const int insertIndex = m_rowsLayout->count() > 0 ? m_rowsLayout->count() - 1 : 0;
            m_rowsLayout->insertWidget(insertIndex, row.container);
            it = m_rows.insert(repoPath, row);
        }

        it->result->setText(resultText);
        it->result->setStyleSheet("QLabel { color: " + color + "; font-weight: bold; }");

        // Rebuild sub-rows in place (children with a layout parent get cleaned up on delete).
        if (it->subContainer)
        {
            QLayout* subLayout = it->subContainer->layout();
            while (QLayoutItem* item = subLayout->takeAt(0))
            {
                if (QWidget* w = item->widget())
                    w->deleteLater();
                delete item;
            }
            for (const SubItem& s : subItems)
            {
                QWidget* subRow = new QWidget(it->subContainer);
                QHBoxLayout* sl = new QHBoxLayout(subRow);
                sl->setContentsMargins(0, 0, 0, 0);
                QLabel* n = new QLabel("• " + s.label, subRow);
                QLabel* r = new QLabel(s.resultText, subRow);
                r->setStyleSheet("QLabel { color: " + s.color + "; font-weight: bold; }");
                QPushButton* logBtn = new QPushButton("Show log", subRow);
                const QString repoKey = repoPath;
                const QString subKey  = s.key;
                connect(logBtn, &QPushButton::clicked, this, [this, repoKey, subKey]() {
                    emit showLogRequested(repoKey, subKey);
                    });
                sl->addWidget(n, 1);
                sl->addWidget(r);
                sl->addWidget(logBtn);
                subLayout->addWidget(subRow);
            }
            it->subContainer->setVisible(!subItems.isEmpty());
        }
    }

    void JobResultDialog::popup()
    {
        show();
        raise();
        activateWindow();
    }
}
