#include "settingchild_general.h"
#include "ui_settingchild_general.h"

#include "../../../GlobalConstants.h"

#include <QSettings>
#include <QSignalBlocker>

namespace
{
//与Dialog默认尺寸保持一致，用于首次打开设置页时展示。
constexpr int kDefaultDialogWidth = 650;
constexpr int kDefaultDialogHeight = 200;
}

SettingChild_General::SettingChild_General(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_General)
{
    ui->setupUi(this);

    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("通用设置");

    //初始化控件时屏蔽信号，避免读取配置时反向写入并触发刷新。
    QSettings settings(IniSettingPath, QSettings::IniFormat);
    const QSignalBlocker widthBlocker(ui->spinBox_DialogWidth);
    const QSignalBlocker heightBlocker(ui->spinBox_DialogHeight);
    ui->spinBox_DialogWidth->setValue(
        settings.value("general/DialogWidth", kDefaultDialogWidth).toInt());
    ui->spinBox_DialogHeight->setValue(
        settings.value("general/DialogHeight", kDefaultDialogHeight).toInt());
}

SettingChild_General::~SettingChild_General()
{
    delete ui;
}

/*设置对话框宽度*/
void SettingChild_General::on_spinBox_DialogWidth_valueChanged(int arg1)
{
    QSettings settings(IniSettingPath, QSettings::IniFormat);
    settings.setValue("general/DialogWidth", arg1);
    emit generalConfigChanged();
}

/*设置对话框高度*/
void SettingChild_General::on_spinBox_DialogHeight_valueChanged(int arg1)
{
    QSettings settings(IniSettingPath, QSettings::IniFormat);
    settings.setValue("general/DialogHeight", arg1);
    emit generalConfigChanged();
}
