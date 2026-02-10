#pragma once
#include <QString>
#include <QStandardPaths>
#include <QDir>

inline const QString Settingpath = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                .filePath("ZcChat2/config.ini");
