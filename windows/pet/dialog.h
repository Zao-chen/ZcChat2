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

    void on_pushButton_next_clicked();

private:
    /*初始化*/
    //鼠标事件
    virtual void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool isLeftPressDown; //判断左键是否按下
    QPoint m_movePoint; //鼠标的位置
    //键盘事件
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    QList<int> keys; //按键按键获取

    /*主逻辑*/
    void initWindow();
    Ui::Dialog *ui;
    AiProvider *ai; //用于AI交互

};

#endif // DIALOG_H
