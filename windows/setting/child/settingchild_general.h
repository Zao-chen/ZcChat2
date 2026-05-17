#ifndef SETTINGCHILD_GENERAL_H
#define SETTINGCHILD_GENERAL_H

#include <QWidget>

namespace Ui
{
class SettingChild_General;
}

class SettingChild_General : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_General(QWidget *parent = nullptr);
    ~SettingChild_General();

  signals:
    void generalConfigChanged(); //通知主窗口刷新通用配置

  private slots:
    void on_spinBox_DialogWidth_valueChanged(int arg1);
    void on_spinBox_DialogHeight_valueChanged(int arg1);

  private:
    Ui::SettingChild_General *ui;
};

#endif //SETTINGCHILD_GENERAL_H
