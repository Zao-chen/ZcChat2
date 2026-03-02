#ifndef ZCSTACKEDWIDGET_H
#define ZCSTACKEDWIDGET_H

#include "ZcWidgetTools_global.h"
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>

class ZCWIDGETTOOLS_EXPORT ZcStackedWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit ZcStackedWidget(QWidget *parent = nullptr);

    void setCurrentIndex(int targetIndex);
    void setCurrentWidget(QWidget* widget);

private:
    bool m_slideAnimating;
    int m_animationDuration;
    double m_offsetFactor;
    QEasingCurve m_easingCurve;

    void setupDefaultCurve();
};

#endif // ZCSTACKEDWIDGET_H
