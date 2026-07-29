#include "ui/TextLogWindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QShortcut>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace CLC
{
    // ------------------------------------------------------------------
    // LineNumberArea + LogPlainTextEdit
    // ------------------------------------------------------------------
    class LineNumberArea : public QWidget
    {
    public:
        explicit LineNumberArea(class LogPlainTextEdit* editor);
        QSize sizeHint() const override;
    protected:
        void paintEvent(QPaintEvent* event) override;
    private:
        LogPlainTextEdit* m_editor;
    };

    class LogPlainTextEdit : public QPlainTextEdit
    {
    public:
        explicit LogPlainTextEdit(QWidget* parent = nullptr);
        int lineNumberAreaWidth() const;
        void lineNumberAreaPaintEvent(QPaintEvent* event);
    protected:
        void resizeEvent(QResizeEvent* event) override;
    private:
        void updateLineNumberAreaWidth();
        void updateLineNumberArea(const QRect& rect, int dy);
        LineNumberArea* m_lineArea;
    };

    LineNumberArea::LineNumberArea(LogPlainTextEdit* editor)
        : QWidget(editor), m_editor(editor) {}

    QSize LineNumberArea::sizeHint() const
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

    void LineNumberArea::paintEvent(QPaintEvent* event)
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

    LogPlainTextEdit::LogPlainTextEdit(QWidget* parent)
        : QPlainTextEdit(parent)
        , m_lineArea(new LineNumberArea(this))
    {
        connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int){ updateLineNumberAreaWidth(); });
        connect(this, &QPlainTextEdit::updateRequest, this,
                [this](const QRect& r, int dy){ updateLineNumberArea(r, dy); });
        updateLineNumberAreaWidth();
    }

    int LogPlainTextEdit::lineNumberAreaWidth() const
    {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) { max /= 10; ++digits; }
        return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }

    void LogPlainTextEdit::updateLineNumberAreaWidth()
    {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    void LogPlainTextEdit::updateLineNumberArea(const QRect& rect, int dy)
    {
        if (dy) m_lineArea->scroll(0, dy);
        else m_lineArea->update(0, rect.y(), m_lineArea->width(), rect.height());
        if (rect.contains(viewport()->rect()))
            updateLineNumberAreaWidth();
    }

    void LogPlainTextEdit::resizeEvent(QResizeEvent* event)
    {
        QPlainTextEdit::resizeEvent(event);
        QRect cr = contentsRect();
        m_lineArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }

    void LogPlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
    {
        QPainter p(m_lineArea);
        p.fillRect(event->rect(), QColor(240, 240, 240));

        QTextBlock block = firstVisibleBlock();
        int number = block.blockNumber();
        int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
        int bottom = top + (int)blockBoundingRect(block).height();
        const int w = m_lineArea->width() - 6;

        p.setPen(QColor(120, 120, 120));
        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top())
            {
                p.drawText(0, top, w, fontMetrics().height(),
                           Qt::AlignRight | Qt::AlignVCenter, QString::number(number + 1));
            }
            block = block.next();
            top = bottom;
            bottom = top + (int)blockBoundingRect(block).height();
            ++number;
        }
    }

    // ------------------------------------------------------------------
    // LogHighlighter — colors PASS/FAIL/ERROR/WARN + separators + testresult
    // ------------------------------------------------------------------
    class LogHighlighter : public QSyntaxHighlighter
    {
    public:
        explicit LogHighlighter(QTextDocument* doc);
        void setEnabled(bool on);
    protected:
        void highlightBlock(const QString& text) override;
    private:
        struct Rule { QRegularExpression pattern; QTextCharFormat format; };
        QVector<Rule> m_rules;
        bool m_enabled = true;
    };

    LogHighlighter::LogHighlighter(QTextDocument* doc)
        : QSyntaxHighlighter(doc)
    {
        auto make = [](const QColor& color, bool bold = false, const QColor& bg = QColor()) {
            QTextCharFormat f;
            f.setForeground(color);
            if (bold) f.setFontWeight(QFont::Bold);
            if (bg.isValid()) f.setBackground(bg);
            return f;
        };

        const QColor green (0x2e, 0xa0, 0x40);
        const QColor red   (0xd6, 0x2c, 0x1a);
        const QColor orange(0xd6, 0x83, 0x10);
        const QColor blue  (0x2f, 0x6f, 0xb0);
        const QColor cyan  (0x0e, 0x7c, 0x86);
        const QColor purple(0x7a, 0x3d, 0xa8);

        // Whole-word test-outcome tokens. Case-insensitive so PASS, Pass, pass all catch.
        auto ci = QRegularExpression::CaseInsensitiveOption;
        m_rules.push_back({ QRegularExpression("\\b(PASS|PASSED|OK|SUCCESS|SUCCESSFUL|SUCCEEDED)\\b", ci),
                            make(green, true) });
        m_rules.push_back({ QRegularExpression("\\b(FAIL|FAILED|FAILURE|FAILING)\\b", ci),
                            make(red, true) });
        m_rules.push_back({ QRegularExpression("\\b(ERROR|ERR|FATAL|EXCEPTION|CRASH|CRASHED|ABORTED)\\b", ci),
                            make(red, true, QColor(0xff, 0xe5, 0xe0)) });
        m_rules.push_back({ QRegularExpression("\\b(WARN|WARNING|DEPRECATED)\\b", ci),
                            make(orange, true) });
        m_rules.push_back({ QRegularExpression("\\b(INFO|NOTE|DEBUG|TRACE)\\b", ci),
                            make(blue) });
        m_rules.push_back({ QRegularExpression("\\b(SKIP|SKIPPED|IGNORED|DISABLED)\\b", ci),
                            make(purple) });
        m_rules.push_back({ QRegularExpression("\\b(canceled|cancelled|canceling|cancelling)\\b", ci),
                            make(orange, true) });

        // "===== <suite> =====" separators emitted by UnitTestRunner.
        m_rules.push_back({ QRegularExpression("^=====.*=====\\s*$"),
                            make(cyan, true, QColor(0xea, 0xf6, 0xf7)) });

        // MSBuild-style "N error(s)" / "N warning(s)" summary lines.
        m_rules.push_back({ QRegularExpression("\\b\\d+\\s+error\\(s\\)", ci),
                            make(red, true) });
        m_rules.push_back({ QRegularExpression("\\b\\d+\\s+warning\\(s\\)", ci),
                            make(orange, true) });

        // Compiler paths like "foo.cpp(123): error C1234:" / "warning C1234:"
        m_rules.push_back({ QRegularExpression("\\berror\\s+[A-Z]\\d+\\b", ci),
                            make(red, true) });
        m_rules.push_back({ QRegularExpression("\\bwarning\\s+[A-Z]\\d+\\b", ci),
                            make(orange, true) });
    }

    void LogHighlighter::setEnabled(bool on)
    {
        if (m_enabled == on) return;
        m_enabled = on;
        rehighlight();
    }

    void LogHighlighter::highlightBlock(const QString& text)
    {
        if (!m_enabled) return;
        for (const Rule& r : m_rules)
        {
            QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
            while (it.hasNext())
            {
                QRegularExpressionMatch m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), r.format);
            }
        }

        // Line-level emphasis: "Testresult: PASS/FAIL" gets a full-line background.
        static const QRegularExpression kTestResult(
            "\\bTestresult\\s*:\\s*(PASS|FAIL|PASSED|FAILED)\\b", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch m = kTestResult.match(text);
        if (m.hasMatch())
        {
            const bool pass = m.captured(1).compare("PASS", Qt::CaseInsensitive) == 0
                           || m.captured(1).compare("PASSED", Qt::CaseInsensitive) == 0;
            QTextCharFormat lf;
            lf.setFontWeight(QFont::Bold);
            lf.setBackground(pass ? QColor(0xd8, 0xf3, 0xdc) : QColor(0xff, 0xd6, 0xd6));
            lf.setForeground(pass ? QColor(0x1a, 0x66, 0x2e) : QColor(0x93, 0x1a, 0x1a));
            setFormat(0, text.length(), lf);
        }
    }

    // ------------------------------------------------------------------
    // TextLogWindow
    // ------------------------------------------------------------------
    TextLogWindow::TextLogWindow(QWidget* parent)
        : QWidget(parent)
        , m_textEdit(new LogPlainTextEdit(this))
    {
        m_textEdit->setReadOnly(true);
        m_textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        m_highlighter = new LogHighlighter(m_textEdit->document());

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 4, 4, 4);
        root->setSpacing(3);

        buildToolbar(root);
        root->addWidget(m_textEdit, 1);
        buildFindBar(root);

        // Footer: cursor position + line/char totals + match count.
        QHBoxLayout* footer = new QHBoxLayout();
        m_statusLabel = new QLabel(this);
        m_matchLabel = new QLabel(this);
        m_matchLabel->setStyleSheet("color: #2f6fb0;");
        footer->addWidget(m_statusLabel, 1);
        footer->addWidget(m_matchLabel);
        root->addLayout(footer);

        connect(m_textEdit, &QPlainTextEdit::cursorPositionChanged, this, &TextLogWindow::updateStatus);
        connect(m_textEdit, &QPlainTextEdit::textChanged, this, &TextLogWindow::updateStatus);

        // Global shortcuts within this window.
        new QShortcut(QKeySequence::Find, this, this, [this]{ openFindBar(); });
        new QShortcut(QKeySequence::Save, this, this, [this]{ saveToFile(); });
        new QShortcut(QKeySequence::FindNext, this, this, [this]{ findNext(true); });
        new QShortcut(QKeySequence::FindPrevious, this, this, [this]{ findNext(false); });

        resize(1000, 650);
        hide();
    }

    void TextLogWindow::buildToolbar(QVBoxLayout* root)
    {
        QToolBar* bar = new QToolBar(this);
        bar->setIconSize(QSize(18, 18));

        QAction* findAct = bar->addAction(QIcon(":/icons/search.png"), "Find");
        findAct->setShortcut(QKeySequence::Find);
        findAct->setToolTip("Find (Ctrl+F)");
        connect(findAct, &QAction::triggered, this, &TextLogWindow::openFindBar);

        QAction* saveAct = bar->addAction(QIcon(":/icons/save.png"), "Save as...");
        saveAct->setShortcut(QKeySequence::Save);
        saveAct->setToolTip("Save log to file (Ctrl+S)");
        connect(saveAct, &QAction::triggered, this, &TextLogWindow::saveToFile);

        QAction* copyAct = bar->addAction(QIcon(":/icons/copy.png"), "Copy all");
        copyAct->setToolTip("Copy the entire log to the clipboard");
        connect(copyAct, &QAction::triggered, this, &TextLogWindow::copyAll);

        bar->addSeparator();

        m_wrapButton = new QToolButton(bar);
        m_wrapButton->setText("Wrap");
        m_wrapButton->setIcon(QIcon(":/icons/wrap.png"));
        m_wrapButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_wrapButton->setCheckable(true);
        m_wrapButton->setToolTip("Toggle word wrap");
        connect(m_wrapButton, &QToolButton::toggled, this, [this](bool on) {
            m_textEdit->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
        });
        bar->addWidget(m_wrapButton);

        m_highlightButton = new QToolButton(bar);
        m_highlightButton->setText("Highlight");
        m_highlightButton->setIcon(QIcon(":/icons/highlighter.png"));
        m_highlightButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_highlightButton->setCheckable(true);
        m_highlightButton->setChecked(true);
        m_highlightButton->setToolTip("Toggle keyword coloring (PASS/FAIL/ERROR/WARN/...)");
        connect(m_highlightButton, &QToolButton::toggled, this, [this](bool on) {
            m_highlighter->setEnabled(on);
        });
        bar->addWidget(m_highlightButton);

        bar->addSeparator();

        QAction* nextErr = bar->addAction(QIcon(":/icons/alert-triangle.png"), "Next issue");
        nextErr->setToolTip("Jump to next ERROR / FAIL / WARNING");
        connect(nextErr, &QAction::triggered, this, [this]() {
            static const QRegularExpression re(
                "\\b(ERROR|FAIL|FAILED|FAILURE|WARN|WARNING|FATAL|CRASH)\\b",
                QRegularExpression::CaseInsensitiveOption);
            const QString all = m_textEdit->toPlainText();
            const int from = m_textEdit->textCursor().position();
            QRegularExpressionMatch m = re.match(all, from + 1);
            if (!m.hasMatch()) m = re.match(all);   // wrap around
            if (m.hasMatch())
            {
                QTextCursor c = m_textEdit->textCursor();
                c.setPosition(m.capturedStart());
                c.setPosition(m.capturedEnd(), QTextCursor::KeepAnchor);
                m_textEdit->setTextCursor(c);
                m_textEdit->ensureCursorVisible();
            }
        });

        QAction* topAct = bar->addAction(QIcon(":/icons/chevron-up.png"), "Top");
        topAct->setToolTip("Jump to top");
        connect(topAct, &QAction::triggered, this, [this]() {
            m_textEdit->moveCursor(QTextCursor::Start);
        });
        QAction* endAct = bar->addAction(QIcon(":/icons/chevron-down.png"), "End");
        endAct->setToolTip("Jump to end");
        connect(endAct, &QAction::triggered, this, [this]() {
            m_textEdit->moveCursor(QTextCursor::End);
        });

        root->addWidget(bar);
    }

    void TextLogWindow::buildFindBar(QVBoxLayout* root)
    {
        m_findBar = new QWidget(this);
        QHBoxLayout* h = new QHBoxLayout(m_findBar);
        h->setContentsMargins(0, 0, 0, 0);

        m_findEdit = new QLineEdit(m_findBar);
        m_findEdit->setPlaceholderText("Find... (Enter=next, Shift+Enter=prev, Esc=close)");
        h->addWidget(m_findEdit, 1);

        QToolButton* prev = new QToolButton(m_findBar); prev->setText("◀");
        prev->setToolTip("Previous match (Shift+Enter)");
        QToolButton* next = new QToolButton(m_findBar); next->setText("▶");
        next->setToolTip("Next match (Enter / F3)");
        h->addWidget(prev); h->addWidget(next);

        m_caseCheck      = new QCheckBox("Aa", m_findBar);       m_caseCheck->setToolTip("Match case");
        m_wholeWordCheck = new QCheckBox("W", m_findBar);        m_wholeWordCheck->setToolTip("Whole word");
        m_regexCheck     = new QCheckBox(".*", m_findBar);       m_regexCheck->setToolTip("Regular expression");
        h->addWidget(m_caseCheck); h->addWidget(m_wholeWordCheck); h->addWidget(m_regexCheck);

        QToolButton* close = new QToolButton(m_findBar); close->setText("✕");
        close->setToolTip("Close (Esc)");
        h->addWidget(close);

        connect(m_findEdit, &QLineEdit::textChanged, this, [this]() {
            countAllMatches();
            findNext(true);
        });
        connect(m_findEdit, &QLineEdit::returnPressed, this, [this]() {
            findNext(!(QApplication::keyboardModifiers() & Qt::ShiftModifier));
        });
        connect(next, &QToolButton::clicked, this, [this]{ findNext(true); });
        connect(prev, &QToolButton::clicked, this, [this]{ findNext(false); });
        connect(close, &QToolButton::clicked, this, [this]{ closeFindBar(); });
        auto refindAll = [this]{ countAllMatches(); findNext(true); };
        connect(m_caseCheck,      &QCheckBox::toggled, this, refindAll);
        connect(m_wholeWordCheck, &QCheckBox::toggled, this, refindAll);
        connect(m_regexCheck,     &QCheckBox::toggled, this, refindAll);

        m_findBar->hide();
        root->addWidget(m_findBar);
    }

    void TextLogWindow::showLog(const QString& title, const QString& text)
    {
        setWindowTitle(title);
        m_textEdit->setPlainText(text);
        m_textEdit->moveCursor(QTextCursor::Start);
        countAllMatches();
        updateStatus();
        show();
        raise();
        activateWindow();
    }

    void TextLogWindow::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Escape && m_findBar->isVisible())
        {
            closeFindBar();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

    void TextLogWindow::openFindBar()
    {
        m_findBar->show();
        m_findEdit->setFocus();
        m_findEdit->selectAll();
    }

    void TextLogWindow::closeFindBar()
    {
        m_findBar->hide();
        m_matchCount = 0;
        m_matchLabel->clear();
        m_textEdit->setFocus();
    }

    QRegularExpression TextLogWindow::buildQuery() const
    {
        QString needle = m_findEdit->text();
        if (needle.isEmpty()) return QRegularExpression();
        if (!m_regexCheck->isChecked())
            needle = QRegularExpression::escape(needle);
        if (m_wholeWordCheck->isChecked())
            needle = "\\b" + needle + "\\b";
        QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
        if (!m_caseCheck->isChecked())
            opts |= QRegularExpression::CaseInsensitiveOption;
        QRegularExpression re(needle, opts);
        return re.isValid() ? re : QRegularExpression();
    }

    void TextLogWindow::findNext(bool forward)
    {
        const QRegularExpression re = buildQuery();
        if (re.pattern().isEmpty()) return;
        const QString all = m_textEdit->toPlainText();
        QTextCursor cur = m_textEdit->textCursor();

        if (forward)
        {
            int from = cur.selectionEnd();
            QRegularExpressionMatch m = re.match(all, from);
            if (!m.hasMatch()) m = re.match(all);   // wrap
            if (m.hasMatch())
            {
                cur.setPosition(m.capturedStart());
                cur.setPosition(m.capturedEnd(), QTextCursor::KeepAnchor);
                m_textEdit->setTextCursor(cur);
                m_textEdit->ensureCursorVisible();
            }
        }
        else
        {
            // Backward: iterate all matches up to the current start, take the last.
            const int limit = cur.selectionStart();
            int lastStart = -1, lastEnd = -1;
            QRegularExpressionMatchIterator it = re.globalMatch(all);
            while (it.hasNext())
            {
                QRegularExpressionMatch m = it.next();
                if (m.capturedStart() < limit)
                {
                    lastStart = m.capturedStart();
                    lastEnd   = m.capturedEnd();
                }
                else break;
            }
            if (lastStart < 0)   // wrap: take the last match anywhere
            {
                QRegularExpressionMatchIterator it2 = re.globalMatch(all);
                while (it2.hasNext())
                {
                    QRegularExpressionMatch m = it2.next();
                    lastStart = m.capturedStart(); lastEnd = m.capturedEnd();
                }
            }
            if (lastStart >= 0)
            {
                cur.setPosition(lastStart);
                cur.setPosition(lastEnd, QTextCursor::KeepAnchor);
                m_textEdit->setTextCursor(cur);
                m_textEdit->ensureCursorVisible();
            }
        }
    }

    void TextLogWindow::countAllMatches()
    {
        const QRegularExpression re = buildQuery();
        if (re.pattern().isEmpty())
        {
            m_matchCount = 0;
            m_matchLabel->clear();
            return;
        }
        int count = 0;
        QRegularExpressionMatchIterator it = re.globalMatch(m_textEdit->toPlainText());
        while (it.hasNext()) { it.next(); ++count; }
        m_matchCount = count;
        m_matchLabel->setText(QString("%1 match%2").arg(count).arg(count == 1 ? "" : "es"));
    }

    void TextLogWindow::updateStatus()
    {
        QTextCursor c = m_textEdit->textCursor();
        const int line = c.blockNumber() + 1;
        const int col = c.positionInBlock() + 1;
        const int totalLines = m_textEdit->blockCount();
        const int totalChars = m_textEdit->document()->characterCount() - 1;   // trailing empty block
        m_statusLabel->setText(QString("Ln %1, Col %2   |   %3 lines, %4 chars")
                                   .arg(line).arg(col).arg(totalLines).arg(qMax(0, totalChars)));
    }

    void TextLogWindow::saveToFile()
    {
        const QString suggested = windowTitle().isEmpty() ? "log.txt" : (windowTitle() + ".txt");
        QString sanitized = suggested;
        sanitized.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        const QString path = QFileDialog::getSaveFileName(this, "Save log", sanitized,
                                                          "Text files (*.txt *.log);;All files (*)");
        if (path.isEmpty()) return;
        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, "Save failed", "Could not open file for writing:\n" + path);
            return;
        }
        QTextStream ts(&f);
        ts << m_textEdit->toPlainText();
        if (!f.commit())
            QMessageBox::warning(this, "Save failed", "Write did not complete:\n" + path);
    }

    void TextLogWindow::copyAll()
    {
        QApplication::clipboard()->setText(m_textEdit->toPlainText());
    }
}
