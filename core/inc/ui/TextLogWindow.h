#pragma once

#include "CmakeLibraryCreator_base.h"
#include <QWidget>

class QPlainTextEdit;

namespace CLC
{
    // Read-only unittest log viewer; app convention plain-QWidget tool window.
    class TextLogWindow : public QWidget
    {
        Q_OBJECT
    public:
        explicit TextLogWindow(QWidget* parent = nullptr);

        void showLog(const QString& title, const QString& text);

    private:
        QPlainTextEdit* m_textEdit;
    };
}
