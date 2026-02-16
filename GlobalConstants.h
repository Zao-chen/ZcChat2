#pragma once
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

//主要是一些可迁移的配置，如APIKey
inline const QString JsonSettingPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .filePath("ZcChat2/config.json");
//一些随机子走的无需迁移的配置，如立绘位置
inline const QString IniSettingPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .filePath("ZcChat2/config.ini");

inline const QString CharacterAssestPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                               .filePath("ZcChat2/Character/Assets");
//读取当前选中的角色
inline QString ReadCharacterTachiePath()
{
    QSettings *settings = new QSettings(IniSettingPath, QSettings::IniFormat);
    QString charName = settings->value("character/CharSelect", "未选择").toString();
    if (charName == "未选择")
        return QString();
    QString tachiePath = QDir(CharacterAssestPath).filePath(charName + "/Tachie");
    if (QDir(tachiePath).exists())
        return tachiePath;
    else
        return QString();
}