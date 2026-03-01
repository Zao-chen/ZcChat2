#pragma once
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

//主要是一些可迁移的配置，如APIKey
#ifdef Q_OS_LINUX //对于linux的适配
inline const QString JsonSettingPath = 
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2/config.json");
#else
inline const QString JsonSettingPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2/config.json");
#endif

//一些随机子走的无需迁移的配置，如立绘位置和大小
inline const QString IniSettingPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2/config.ini");

//角色资源位置
inline const QString CharacterAssestPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2/Character/Assets");

//角色配置位置
inline const QString CharacterUserConfigPath =
    QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath("ZcChat2/Character/UserConfig");

//读取当前选中的角色
inline QString ReadNowSelectChar()
{
    QSettings *settings = new QSettings(IniSettingPath, QSettings::IniFormat);
    QString charName =
        settings->value("character/CharSelect", "未选择").toString();
    return charName;
}

//读取当前选中角色的立绘路径
inline QString ReadCharacterTachiePath()
{
    if (ReadNowSelectChar() == "未选择")
        return QString();
    QString tachiePath =
        QDir(CharacterAssestPath).filePath(ReadNowSelectChar() + "/Tachie");
    if (QDir(tachiePath).exists())
        return tachiePath;
    else
        return QString();
}

//读取当前选中角色的配置路径
inline QString ReadCharacterUserConfigPath()
{
    if (ReadNowSelectChar() == "未选择")
        return QString();
    QString tachiePath = QDir(CharacterUserConfigPath)
                             .filePath(ReadNowSelectChar() + "/config.json");
    return tachiePath;
}