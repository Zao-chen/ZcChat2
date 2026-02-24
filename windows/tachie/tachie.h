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
    void requestToggleVisible(); //切换对话框的显示状态

public slots:
    void SetCharTachie(QString TachieName = "default");
    void SetTachieSize(int size);

private:
    Ui::Tachie *ui;
    QPixmap NowTachie;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        emit requestToggleVisible();  //发出信号
    }
};

#endif // TACHIE_H
