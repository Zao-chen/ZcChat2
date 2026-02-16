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

public slots:
    void ToggleVisible();

private slots:
    void on_pushButton_next_clicked();

signals:
    void requestSetCharTachie(QString TachieName);

private:
    /*初始化*/
    //鼠标事件
    virtual void paintEvent(QPaintEvent *event) override;
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
