#pragma once
#include <QString>
#include <QStandardPaths>
#include <QDir>

inline const QString SettingPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .filePath("ZcChat2/config.ini");
inline const QString CharacterTachiePath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                       .filePath("ZcChat2/Character/test/Tachie");