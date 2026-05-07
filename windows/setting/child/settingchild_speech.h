#ifndef SETTINGCHILD_SPEECH_H
#define SETTINGCHILD_SPEECH_H

#include <QWidget>

namespace Ui
{
class SettingChild_Speech;
}

class SettingChild_Speech : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_Speech(QWidget *parent = nullptr);
    ~SettingChild_Speech();

  signals:
    void speechConfigChanged();

  private slots:
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb,
                                            QStringList lastBreadcrumbList);
    void on_pushButton_Baidu_Set_clicked();
    void on_ToggleSwitch_SpeechInputEnable_toggled(bool checked);
    void on_lineEdit_BaiduApiKey_textChanged(const QString &arg1);
    void on_lineEdit_BaiduSecretKey_textChanged(const QString &arg1);

  private:
    Ui::SettingChild_Speech *ui;
};

#endif //SETTINGCHILD_SPEECH_H
