#ifndef DIALOG_H
#define DIALOG_H

#include "AiProvider.h"
#include <QWidget>

namespace Ui {
class Dialog;
}

class Dialog : public QWidget
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:

private:
    /*初始化*/
    virtual void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool isLeftPressDown; //判断左键是否按下
    QPoint m_movePoint; //鼠标的位置
    void initWindow();
    /*主逻辑*/
    Ui::Dialog *ui;
    AiProvider *ai; //用于AI交互

};

#endif // DIALOG_H
