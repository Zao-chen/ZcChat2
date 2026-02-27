#include "settingchild_vits.h"
#include "ui_settingchild_vits.h"

SettingChild_Vits::SettingChild_Vits(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_Vits)
{
    ui->setupUi(this);
    /*初始化*/
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("语音合成设置");
}

SettingChild_Vits::~SettingChild_Vits()
{
    delete ui;
}
