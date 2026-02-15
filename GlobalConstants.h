#pragma once
#include <QString>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

inline const QString SettingPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .filePath("ZcChat2/config.ini");
inline const QString CharacterAssestPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                               .filePath("ZcChat2/Character/Assets");
//读取当前选中的角色

inline QString ReadCharacterTachiePath()
{
    QSettings *settings = new QSettings(SettingPath, QSettings::IniFormat);
    QString charName = settings->value("character/CharSelect", "未选择").toString();
    if (charName == "未选择")
        return QString();
    QString tachiePath = QDir(CharacterAssestPath).filePath(charName + "/Tachie");
    if (QDir(tachiePath).exists())
        return tachiePath;
    else
        return QString();
}