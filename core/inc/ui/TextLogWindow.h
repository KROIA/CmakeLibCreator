#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>
#include <QRegularExpression>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QToolButton;
class QVBoxLayout;

namespace CLC
{
    class LogPlainTextEdit;    // internal: QPlainTextEdit + line-number gutter
    class LogHighlighter;      // internal: QSyntaxHighlighter for keywords/patterns

    // Read-only log viewer with find, syntax highlighting, line numbers, save-to-file.
    // App convention: plain-QWidget tool window.
    class TextLogWindow : public QWidget
    {
        Q_OBJECT
    public:
        explicit TextLogWindow(QWidget* parent = nullptr);

        void showLog(const QString& title, const QString& text);

    protected:
        void keyPressEvent(QKeyEvent* event) override;

    private:
        void buildToolbar(QVBoxLayout* root);
        void buildFindBar(QVBoxLayout* root);
        void updateStatus();                   // refreshes the footer (line count, cursor pos, match count)
        void openFindBar();
        void closeFindBar();
        void findNext(bool forward);
        void countAllMatches();                // updates m_matchCount for the current query
        QRegularExpression buildQuery() const; // honors regex/case/whole-word toggles; empty regex if invalid
        void saveToFile();
        void copyAll();

        LogPlainTextEdit* m_textEdit = nullptr;
        LogHighlighter*   m_highlighter = nullptr;
        QWidget*   m_findBar = nullptr;
        QLineEdit* m_findEdit = nullptr;
        QCheckBox* m_caseCheck = nullptr;
        QCheckBox* m_wholeWordCheck = nullptr;
        QCheckBox* m_regexCheck = nullptr;
        QToolButton* m_wrapButton = nullptr;
        QToolButton* m_highlightButton = nullptr;
        QLabel* m_statusLabel = nullptr;
        QLabel* m_matchLabel = nullptr;
        int m_matchCount = 0;
    };
}
