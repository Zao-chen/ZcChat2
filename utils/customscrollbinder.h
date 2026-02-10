#pragma once

#include <QObject>
#include <QTextEdit>
#include <QScrollBar>

class CustomScrollBinder : public QObject
{
    Q_OBJECT
public:
    explicit CustomScrollBinder(
        QTextEdit *textEdit,
        QScrollBar *customScrollBar,
        int scale = 5,
        QObject *parent = nullptr
        );

private:
    QTextEdit  *m_textEdit;
    QScrollBar *m_customScrollBar;
    int m_scale;
};
