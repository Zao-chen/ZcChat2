#ifndef ZCJSONLIB_H
#define ZCJSONLIB_H

#include "ZcJsonLib_global.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonValue>

class ZCJSONLIB_EXPORT ZcJsonLib
{
public:
    explicit ZcJsonLib(const QString &filePath);

    bool load();
    bool save();

    void setValue(const QString &key, const QJsonValue &value);
    QJsonValue value(const QString &key,
                     const QJsonValue &defaultValue = QJsonValue()) const;

private:
    QString m_filePath;
    QJsonObject m_root;

    QStringList splitKey(const QString &key) const;
};

#endif
