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
    void on_pushButton_SendTest_clicked();

private:
    Ui::Dialog *ui;
    AiProvider *ai; //用于AI交互
};

#endif // DIALOG_H
