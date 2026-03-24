#ifndef TACHIE_H
#define TACHIE_H

#include <QWidget>

namespace Ui
{
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
    void SetTachieImg(QString TachieName = "default");
    void SetTachieSize(int size);
    void ResetTachieLoc();

  private:
    Ui::Tachie *ui;
    QPixmap NowTachie;
    QImage _scaledImg; //用于缓存缩放后的图片，避免编译版本差异

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        emit requestToggleVisible(); //发出信号
    }

    void mousePressEvent(QMouseEvent *event) override; //为了实现鼠标穿透
};

#endif //TACHIE_H
