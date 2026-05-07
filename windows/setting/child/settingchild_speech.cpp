#include "settingchild_speech.h"
#include "ui_settingchild_speech.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

SettingChild_Speech::SettingChild_Speech(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_Speech)
{
    ui->setupUi(this);

    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("语音输入设置");

    ZcJsonLib config(JsonSettingPath);
    ui->ToggleSwitch_SpeechInputEnable->setIsToggled(
        config.value("speechInput/Enable", false).toBool());
}

SettingChild_Speech::~SettingChild_Speech()
{
    delete ui;
}

/*面包屑导航*/
void SettingChild_Speech::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(breadcrumb)
    Q_UNUSED(lastBreadcrumbList)
    ui->stackedWidget->setCurrentIndex(0);
}

/*进入下一级*/
void SettingChild_Speech::on_pushButton_Baidu_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("百度语音识别");

    ZcJsonLib config(JsonSettingPath);
    ui->lineEdit_BaiduApiKey->setText(
        config.value("speechInput/Baidu/ApiKey").toString());
    ui->lineEdit_BaiduSecretKey->setText(
        config.value("speechInput/Baidu/SecretKey").toString());
}

/*启用输入*/
void SettingChild_Speech::on_ToggleSwitch_SpeechInputEnable_toggled(bool checked)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Enable", checked);
    emit speechConfigChanged();
}

void SettingChild_Speech::on_lineEdit_BaiduApiKey_textChanged(const QString &arg1)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Baidu/ApiKey", arg1);
    emit speechConfigChanged();
}

void SettingChild_Speech::on_lineEdit_BaiduSecretKey_textChanged(
    const QString &arg1)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("speechInput/Baidu/SecretKey", arg1);
    emit speechConfigChanged();
}
