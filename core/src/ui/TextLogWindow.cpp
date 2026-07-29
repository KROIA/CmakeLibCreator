#include "ui/TextLogWindow.h"

#include <QFontDatabase>
#include <QPlainTextEdit>
#include <QVBoxLayout>

namespace CLC
{
    TextLogWindow::TextLogWindow(QWidget* parent)
        : QWidget(parent)
        , m_textEdit(new QPlainTextEdit(this))
    {
        m_textEdit->setReadOnly(true);
        m_textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

        QVBoxLayout* root = new QVBoxLayout(this);
        root->addWidget(m_textEdit);

        resize(900, 600);
        hide();
    }

    void TextLogWindow::showLog(const QString& title, const QString& text)
    {
        setWindowTitle(title);
        m_textEdit->setPlainText(text);
        show();
        raise();
        activateWindow();
    }
}
