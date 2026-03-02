// DragHelper.cpp
#include "DragHelper.h"

DragHelper::DragHelper(QWidget *parent)
    : QObject(parent), m_widget(parent)
{
    m_widget->installEventFilter(this);
}

bool DragHelper::eventFilter(QObject *watched, QEvent *event)
{
    if(watched != m_widget) return false;

    switch(event->type())
    {
    case QEvent::MouseButtonPress:
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        if(e->button() == Qt::LeftButton)
        {
            isLeftPressDown = true;
            m_movePoint = e->globalPosition().toPoint() - m_widget->frameGeometry().topLeft();
            m_widget->grabMouse();
            m_widget->setCursor(Qt::SizeAllCursor);
        }
        break;
    }
    case QEvent::MouseMove:
    {
        if(isLeftPressDown)
        {
            QMouseEvent *e = static_cast<QMouseEvent*>(event);
            m_widget->move(e->globalPosition().toPoint() - m_movePoint);
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease:
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        if(e->button() == Qt::LeftButton)
        {
            isLeftPressDown = false;
            m_widget->releaseMouse();
            m_widget->setCursor(Qt::ArrowCursor);
        }
        break;
    }
    default:
        break;
    }
    return false;
}
