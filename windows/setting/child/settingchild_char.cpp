#include "settingchild_char.h"
#include "ui_settingchild_char.h"

#include "../../../GlobalConstants.h"

#include <QSettings>

SettingChild_Char::SettingChild_Char(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_Char)
{
    ui->setupUi(this);
    RefreshCharList();
    //设置默认项
    QSettings *settings = new QSettings(SettingPath, QSettings::IniFormat, this);
    QString defaultChar = settings->value("character/CharSelect", "未选择").toString();
    ui->comboBox_CharList->setCurrentText("defaultChar");
    ui->BreadcrumbBar->appendBreadcrumb("角色设置");
    ui->BreadcrumbBar->setTextPixelSize(25);
}

SettingChild_Char::~SettingChild_Char()
{
    delete ui;
}

void SettingChild_Char::on_pushButton_RefreshCharList_clicked()
{
    RefreshCharList();
}

void SettingChild_Char::RefreshCharList()
{
    //获取所有角色文件夹并添加到combox
    QDir charDir(CharacterAssestPath);
    QStringList charFolders = charDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->comboBox_CharList->clear();
    ui->comboBox_CharList->addItems(charFolders);
}


void SettingChild_Char::on_comboBox_CharList_currentTextChanged(const QString &arg1)
{
    //保存到配置文件
    QSettings *settings = new QSettings(SettingPath, QSettings::IniFormat, this);
    settings->setValue("character/CharSelect", ui->comboBox_CharList->currentText());
    emit requestReloadCharSelect();
}

