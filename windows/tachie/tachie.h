#ifndef TACHIE_H
#define TACHIE_H

#include <QWidget>

namespace Ui {
class Tachie;
}

class Tachie : public QWidget
{
    Q_OBJECT

public:
    explicit Tachie(QWidget *parent = nullptr);
    ~Tachie();

signals:
    void toggleVisible();  // 信号：请求切换窗口B

public slots:
    void ReloadCharSelect();

private:
    Ui::Tachie *ui;


protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        emit toggleVisible();  // 发出信号，通知主窗口切换窗口B的可见性
    }
};

#endif // TACHIE_H
