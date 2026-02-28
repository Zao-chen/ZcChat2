#ifndef SETTINGCHILD_VITS_H
#define SETTINGCHILD_VITS_H

#include <QWidget>

namespace Ui
{
class SettingChild_Vits;
}

class SettingChild_Vits : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_Vits(QWidget *parent = nullptr);
    ~SettingChild_Vits();

  private slots:
    void on_lineEdit_ApiUrl_textChanged(const QString &arg1);
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList);
    void on_pushButton_VSA_Set_clicked();

    void on_pushButton_LoadModelAndSpeakerlList_clicked();

  private:
    Ui::SettingChild_Vits *ui;
};

#endif // SETTINGCHILD_VITS_H
