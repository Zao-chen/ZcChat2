// DragHelper.h
#pragma once
#include <QObject>
#include <QWidget>
#include <QMouseEvent>

class DragHelper : public QObject
{
    Q_OBJECT
public:
    explicit DragHelper(QWidget *parent);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *m_widget;
    bool isLeftPressDown = false;
    QPoint m_movePoint;
};
