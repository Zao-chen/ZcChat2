#include "AnimePlugin.h"
#include "ZcJsonLib.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

namespace
{
/*解析单个动画步骤*/
bool ParseStep(const QJsonObject &stepObj, AnimePluginStep &outStep,
               QString &outError)
{
    const QString type = stepObj.value("type").toString().trimmed();

    //水平移动
    if (type == "move")
    {
        //move为相对位移，x/y可正可负
        outStep.type = AnimePluginStep::Type::Move;
        outStep.durationSec = stepObj.value("duration").toDouble(-1.0);
        outStep.x = stepObj.value("x").toDouble(0.0);
        outStep.y = stepObj.value("y").toDouble(0.0);
        if (outStep.durationSec <= 0.0)
        {
            outError = "move 步骤的 duration 必须大于 0";
            return false;
        }
        return true;
    }
    //透明度
    if (type == "opacity")
    {
        //opacity限制在0~1，避免无效透明度
        outStep.type = AnimePluginStep::Type::Opacity;
        outStep.durationSec = stepObj.value("duration").toDouble(-1.0);
        outStep.from = stepObj.value("from").toDouble(-1.0);
        outStep.to = stepObj.value("to").toDouble(-1.0);
        if (outStep.durationSec <= 0.0)
        {
            outError = "opacity 步骤的 duration 必须大于 0";
            return false;
        }
        if (outStep.from < 0.0 || outStep.from > 1.0 || outStep.to < 0.0 ||
            outStep.to > 1.0)
        {
            outError = "opacity 步骤的 from/to 必须在 0~1 之间";
            return false;
        }
        return true;
    }
    //缩放
    if (type == "scale")
    {
        //scale使用倍率，必须大于0
        outStep.type = AnimePluginStep::Type::Scale;
        outStep.durationSec = stepObj.value("duration").toDouble(-1.0);
        outStep.scaleFrom = stepObj.value("from").toDouble(-1.0);
        outStep.scaleTo = stepObj.value("to").toDouble(-1.0);
        if (outStep.durationSec <= 0.0)
        {
            outError = "scale 步骤的 duration 必须大于 0";
            return false;
        }
        if (outStep.scaleFrom <= 0.0 || outStep.scaleTo <= 0.0)
        {
            outError = "scale 步骤的 from/to 必须大于 0";
            return false;
        }
        return true;
    }

    outError = QString("不支持的步骤类型: %1").arg(type);
    return false;
}

/*解析完整动画*/
bool ParseAnimation(const QJsonObject &animationObj,
                    AnimePluginAnimation &outAnimation, QString &outError)
{
    outAnimation.name = animationObj.value("name").toString().trimmed();
    if (outAnimation.name.isEmpty())
    {
        outError = "动画缺少 name";
        return false;
    }

    const QJsonArray steps = animationObj.value("steps").toArray();
    if (steps.isEmpty())
    {
        outError = QString("动画 %1 的 steps 不能为空").arg(outAnimation.name);
        return false;
    }

    outAnimation.steps.clear();
    outAnimation.steps.reserve(steps.size());
    for (int i = 0; i < steps.size(); ++i)
    {
        if (!steps.at(i).isObject())
        {
            outError =
                QString("动画 %1 的第 %2 个步骤不是对象")
                    .arg(outAnimation.name)
                    .arg(i + 1);
            return false;
        }

        AnimePluginStep step;
        if (!ParseStep(steps.at(i).toObject(), step, outError))
        {
            outError =
                QString("动画 %1 的第 %2 个步骤无效: %3")
                    .arg(outAnimation.name)
                    .arg(i + 1)
                    .arg(outError);
            return false;
        }
        outAnimation.steps.append(step); //写入动画步骤列表
    }

    return true;
}
} //namespace

/*读取插件*/
bool LoadAnimePluginFromFile(const QString &filePath,
                             AnimePluginDefinition &outPlugin,
                             QString &outError)
{
    ZcJsonLib pluginConfig(filePath);
    if (!pluginConfig.load())
    {
        outError = QString("无法读取插件文件: %1").arg(filePath);
        return false;
    }

    /*解析插件信息*/
    outPlugin = AnimePluginDefinition();
    outPlugin.filePath = filePath;
    outPlugin.name = pluginConfig.value("name").toString().trimmed();
    outPlugin.version = pluginConfig.value("version").toString().trimmed();
    outPlugin.author = pluginConfig.value("author").toString().trimmed();
    outPlugin.link = pluginConfig.value("link").toString().trimmed();

    //插件元信息必须完整，供后续导入展示与索引使用
    if (outPlugin.name.isEmpty() || outPlugin.version.isEmpty() ||
        outPlugin.author.isEmpty() || outPlugin.link.isEmpty())
    {
        outError = "插件必须包含 name/version/author/link";
        return false;
    }

    /*解析动画列表*/
    const QJsonArray animations = pluginConfig.value("animations").toArray();
    if (animations.isEmpty())
    {
        outError = "animations 不能为空";
        return false;
    }

    QSet<QString> animationNameSet;
    outPlugin.animations.reserve(animations.size());
    for (int i = 0; i < animations.size(); ++i)
    {
        if (!animations.at(i).isObject())
        {
            outError = QString("第 %1 个动画不是对象").arg(i + 1);
            return false;
        }

        AnimePluginAnimation animation;
        if (!ParseAnimation(animations.at(i).toObject(), animation, outError))
            return false;

        //同一插件内动画name必须唯一
        if (animationNameSet.contains(animation.name))
        {
            outError = QString("动画 name 重复: %1").arg(animation.name);
            return false;
        }

        animationNameSet.insert(animation.name);
        outPlugin.animations.append(animation);
    }

    return true;
}
