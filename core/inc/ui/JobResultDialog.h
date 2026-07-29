#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>
#include <QMap>
#include <QVector>

class QVBoxLayout;
class QLabel;

namespace CLC
{
    // Result popup listing one row per repo of a run (build or unittest):
    // repo name + colored result label + "Show log" button. App convention plain-QWidget tool window.
    // Unittest rows can also carry per-suite sub-rows (one per executable) with their own log buttons.
    class JobResultDialog : public QWidget
    {
        Q_OBJECT
    public:
        struct SubItem
        {
            QString key;         // opaque id passed back in showLogRequested (e.g. suite name)
            QString label;
            QString resultText;
            QString color;
        };

        explicit JobResultDialog(const QString& title, QWidget* parent = nullptr);

        void markRunning(const QString& repoName, const QString& repoPath); // add row or update in place; sets "Running..." label
        void setResult(const QString& repoName, const QString& repoPath,    // add row or update in place (keyed by repoPath)
                       const QString& resultText, const QString& color,
                       const QVector<SubItem>& subItems = {});
        void popup();                                                       // show(); raise(); activateWindow();

    signals:
        // Row button clicked. subKey is empty for the repo's combined-log button
        // and set to a SubItem::key for per-suite log buttons.
        void showLogRequested(const QString& repoPath, const QString& subKey);
        // Retry button clicked — re-run the build/unittest for this repo.
        void retryRequested(const QString& repoPath);

    private:
        struct Row
        {
            QWidget* container = nullptr;
            QLabel*  result = nullptr;
            QWidget* subContainer = nullptr;   // holds per-suite sub-rows; rebuilt each setResult
        };

        Row& ensureRow(const QString& repoName, const QString& repoPath);

        QVBoxLayout* m_rowsLayout = nullptr;   // rows inserted above trailing stretch
        QMap<QString, Row> m_rows;             // key: repoPath
    };
}
