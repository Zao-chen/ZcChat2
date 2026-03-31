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
    QImage _scaledImg;                  //用于缓存缩放后的图片，避免编译版本差异
    bool _tachiePosRestoreDone = false; //位置恢复完成后才允许自动保存
    void SaveTachieLoc();               //将当前立绘位置写入 config.ini（按角色）
    void RestoreTachieLoc();            //从 config.ini 读取并恢复立绘位置

  protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        emit requestToggleVisible(); //发出信号
    }

    void mousePressEvent(QMouseEvent *event) override; //为了实现鼠标穿透
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif //TACHIE_H
