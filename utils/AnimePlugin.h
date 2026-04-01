#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct AnimePluginStep
{
    enum class Type
    {
        Move,
        Opacity,
        Scale
    };

    Type type = Type::Move;
    double durationSec = 0.0;

    //move:相对位移
    double x = 0.0;
    double y = 0.0;

    //opacity:透明度变化
    double from = 1.0;
    double to = 1.0;

    //scale:缩放变化（1.0 = 100%）
    double scaleFrom = 1.0;
    double scaleTo = 1.0;
};

struct AnimePluginAnimation
{
    QString id;
    QString name;
    QList<AnimePluginStep> steps;

    QString BuildDisplayName(const QString &pluginName) const
    {
        return pluginName + "_" + name;
    }

    QString BuildUniqueKey(const QString &pluginId) const
    {
        return pluginId + ":" + id;
    }
};

struct AnimePluginDefinition
{
    QString filePath;
    QString schema;
    QString pluginId;
    QString name;
    QString author;
    QString link;
    QList<AnimePluginAnimation> animations;
};

class AnimePluginLoader
{
  public:
    static bool LoadFromFile(const QString &filePath,
                             AnimePluginDefinition &outPlugin,
                             QString &outError);

    static QStringList BuildAnimationDisplayNames(
        const AnimePluginDefinition &plugin);
};
