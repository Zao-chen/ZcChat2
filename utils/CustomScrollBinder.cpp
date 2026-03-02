#include "CustomScrollBinder.h"
#include <QSignalBlocker>

CustomScrollBinder::CustomScrollBinder(
    QTextEdit *textEdit,
    QScrollBar *customScrollBar,
    int scale,
    QObject *parent
    )
    : QObject(parent)
    , m_textEdit(textEdit)
    , m_customScrollBar(customScrollBar)
    , m_scale(scale)
{
    if (!m_textEdit || !m_customScrollBar)
        return;

    m_customScrollBar->hide();

    QScrollBar *scroll = m_textEdit->verticalScrollBar();

    // 范围同步
    connect(scroll, &QScrollBar::rangeChanged, this,
            [=](int min, int max) {
                QSignalBlocker blocker(m_customScrollBar);
                m_customScrollBar->setRange(min, max / m_scale);
                m_customScrollBar->setPageStep(scroll->pageStep() / m_scale);
                m_customScrollBar->setSingleStep(qMax(1, scroll->singleStep() / m_scale));
                m_customScrollBar->setVisible(max > min);
            });

    // 拖动自定义滚动条
    connect(m_customScrollBar, &QScrollBar::sliderMoved, this,
            [=](int value) {
                QSignalBlocker blocker(scroll);
                scroll->setValue(value * m_scale);
                m_textEdit->viewport()->update();
            });

    // 点击空白区域
    connect(m_customScrollBar, &QScrollBar::actionTriggered, this,
            [=](int action) {
                if (action == QScrollBar::SliderPageStepAdd ||
                    action == QScrollBar::SliderPageStepSub) {
                    QSignalBlocker blocker(scroll);
                    scroll->setValue(m_customScrollBar->value() * m_scale);
                    m_textEdit->viewport()->update();
                }
            });

    // textEdit → 自定义滚动条
    connect(scroll, &QScrollBar::valueChanged, this,
            [=](int value) {
                QSignalBlocker blocker(m_customScrollBar);
                m_customScrollBar->setValue(value / m_scale);
            });
}
