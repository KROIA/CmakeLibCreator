#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>
#include <QMap>

class QVBoxLayout;
class QLabel;

namespace CLC
{
    // Result popup listing one row per repo of a run (build or unittest):
    // repo name + colored result label + "Show log" button. App convention plain-QWidget tool window.
    class JobResultDialog : public QWidget
    {
        Q_OBJECT
    public:
        explicit JobResultDialog(const QString& title, QWidget* parent = nullptr);

        void beginRun();                                                    // clears all rows (new run reset)
        void setResult(const QString& repoName, const QString& repoPath,    // add row or update in place (keyed by repoPath)
                       const QString& resultText, const QString& color);
        void popup();                                                       // show(); raise(); activateWindow();

    signals:
        void showLogRequested(const QString& repoPath);                     // row button clicked

    private:
        struct Row
        {
            QWidget* container = nullptr;
            QLabel*  result = nullptr;
        };

        QVBoxLayout* m_rowsLayout = nullptr;   // rows inserted above trailing stretch
        QMap<QString, Row> m_rows;             // key: repoPath
    };
}
