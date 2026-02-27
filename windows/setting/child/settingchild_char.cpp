#include "settingchild_char.h"
#include "ui_settingchild_char.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include <QSettings>
#include <QJsonArray>

SettingChild_Char::SettingChild_Char(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_Char)
{
    ui->setupUi(this);
    RefreshCharList();
    ui->BreadcrumbBar->appendBreadcrumb("角色设置");
    ui->BreadcrumbBar->setTextPixelSize(25);

    /*读取配置项*/
    QString charName = ui->comboBox_CharList->currentText();
    //角色选择
    QSettings *settings = new QSettings(IniSettingPath, QSettings::IniFormat, this);
    QString defaultChar = settings->value("character/CharSelect", "未选择").toString();
    ui->comboBox_CharList->setCurrentText("defaultChar");
    //读取角色提示词
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    QString charPrompt = charConfig.value("prompt").toString();
    ui->plainTextEdit_CharPrompt->setPlainText(charPrompt);
    //读取立绘大小
    ZcJsonLib charUserConfig(CharacterUserConfigPath + "/" + charName + "/config.json");
    QString tachieSize = charUserConfig.value("tachieSize").toString();
    ui->spinBox_TachieSize->setValue(tachieSize.toInt());
    //读取服务商选择
    QString serverSelect = charUserConfig.value("serverSelect").toString();
    ui->comboBox_ServerSelect->setCurrentText(serverSelect);
    RefreshModelList();
    //读取模型选择
    QString modelSelect = charUserConfig.value("modelSelect").toString();
    ui->comboBox_ModelSelect->setCurrentText(modelSelect);
    /*Vits*/
    //读取模型角色列表到comboBox_Vits_MASSelect
    ZcJsonLib config(JsonSettingPath);
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    QStringList vitsMasList;
    for (const QJsonValue& val : arr)  vitsMasList.append(val.toString());
    ui->comboBox_Vits_MASSelect->addItems(vitsMasList);
    //读取语音合成模型选择
    QString vitsMasSelect = charUserConfig.value("vitsMasSelect").toString();
    ui->comboBox_Vits_MASSelect->setCurrentText(vitsMasSelect);
}

SettingChild_Char::~SettingChild_Char()
{
    delete ui;
}

void SettingChild_Char::on_pushButton_RefreshCharList_clicked()
{
    RefreshCharList();
}

//刷新角色列表
void SettingChild_Char::RefreshCharList()
{
    //获取所有角色文件夹并添加到combox
    QDir charDir(CharacterAssestPath);
    QStringList charFolders = charDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->comboBox_CharList->clear();
    ui->comboBox_CharList->addItems(charFolders);
}

//修改选中角色
void SettingChild_Char::on_comboBox_CharList_currentTextChanged(const QString &arg1)
{
    //保存到配置文件
    QSettings *settings = new QSettings(IniSettingPath, QSettings::IniFormat, this);
    settings->setValue("character/CharSelect", ui->comboBox_CharList->currentText());
    emit requestReloadCharSelect("default");
}

//修改角色提示词
void SettingChild_Char::on_plainTextEdit_CharPrompt_textChanged()
{
    //保存到角色文件夹下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString charPrompt = ui->plainTextEdit_CharPrompt->toPlainText();
    ZcJsonLib charConfig(CharacterAssestPath + "/" + charName + "/config.json");
    charConfig.setValue("prompt", charPrompt);
}

//修改立绘大小
void SettingChild_Char::on_spinBox_TachieSize_textChanged(const QString &arg1)
{
    //保存到角色配置位置下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString tachieSize = ui->spinBox_TachieSize->text();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("tachieSize", tachieSize);
    emit requestSetTachieSize(arg1.toInt());
}

//切换服务商选择
void SettingChild_Char::on_comboBox_ServerSelect_currentTextChanged(const QString &arg1)
{
    //保存到角色配置位置下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("serverSelect", serverSelect);
    RefreshModelList();
    emit requestReloadAIConfig();
}

//刷新模型列表
void SettingChild_Char::RefreshModelList()
{
    QString serverSelect = ui->comboBox_ServerSelect->currentText();
    ZcJsonLib config(JsonSettingPath);
    QStringList modelList;
    QJsonArray modelArray = config.value("llm/" + serverSelect + "/ModelList").toArray();
    for (const auto &model : modelArray)
    {
        modelList.append(model.toString());
    }
    ui->comboBox_ModelSelect->clear();
    ui->comboBox_ModelSelect->addItems(modelList);
}

//切换模型选择
void SettingChild_Char::on_comboBox_ModelSelect_currentTextChanged(const QString &arg1)
{
    //保存到角色配置位置下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString modelSelect = ui->comboBox_ModelSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("modelSelect", modelSelect);
    emit requestReloadAIConfig();
}

//重置立绘位置
void SettingChild_Char::on_pushButton_ResetTachieLoc_clicked()
{
    emit requestResetTachieLoc();
}

//切换语音合成模型选择
void SettingChild_Char::on_comboBox_Vits_MASSelect_currentTextChanged(const QString &arg1)
{
    //保存到角色配置位置下的config.json
    QString charName = ui->comboBox_CharList->currentText();
    QString vitsMasSelect = ui->comboBox_Vits_MASSelect->currentText();
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    charConfig.setValue("vitsMasSelect", vitsMasSelect);
    emit requestReloadAIConfig();
}

