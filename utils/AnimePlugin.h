#pragma once

#include <QList>
#include <QString>
#include <QStringList>

//动画插件描述与加载器
//此文件定义了用于描述动画插件的数据结构以及从 JSON 文件加载

/*单个步骤结构体*/
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

    //move: 相对位移（像素或单位），相对于当前坐标的偏移
    double x = 0.0;
    double y = 0.0;

    //opacity: 透明度变化，取值范围 0.0 ~ 1.0
    double from = 1.0;
    double to = 1.0;

    //scale: 缩放变化（1.0 = 100%），值必须大于 0
    double scaleFrom = 1.0;
    double scaleTo = 1.0;
};

/*完整动画结构体*/
struct AnimePluginAnimation
{
    QString name;
    QList<AnimePluginStep> steps;

    QString BuildUniqueKey(const QString &pluginName) const
    {
        return pluginName + "_" + name;
    }
};

/*动画插件信息结构体*/
struct AnimePluginDefinition
{
    QString filePath;
    QString name;
    QString version;
    QString author;
    QString link;
    QList<AnimePluginAnimation> animations;
};

/*从文件加载动画插件*/
bool LoadAnimePluginFromFile(const QString &filePath,
                             AnimePluginDefinition &outPlugin,
                             QString &outError);
