#include "AnimePlugin.h"
#include "ZcJsonLib.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>

namespace
{
//当前仅支持的插件schema版本
constexpr const char *kSchemaV1 = "zcchat2.anime-plugin/v1";

bool ParseStep(const QJsonObject &stepObj, AnimePluginStep &outStep,
               QString &outError)
{
    const QString type = stepObj.value("type").toString().trimmed();
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

bool ParseAnimation(const QJsonObject &animationObj,
                    AnimePluginAnimation &outAnimation, QString &outError)
{
    outAnimation.id = animationObj.value("id").toString().trimmed();
    outAnimation.name = animationObj.value("name").toString().trimmed();
    if (outAnimation.id.isEmpty() || outAnimation.name.isEmpty())
    {
        outError = "动画缺少 id 或 name";
        return false;
    }

    const QJsonArray steps = animationObj.value("steps").toArray();
    if (steps.isEmpty())
    {
        outError = QString("动画 %1 的 steps 不能为空").arg(outAnimation.id);
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
                    .arg(outAnimation.id)
                    .arg(i + 1);
            return false;
        }

        AnimePluginStep step;
        if (!ParseStep(steps.at(i).toObject(), step, outError))
        {
            outError =
                QString("动画 %1 的第 %2 个步骤无效: %3")
                    .arg(outAnimation.id)
                    .arg(i + 1)
                    .arg(outError);
            return false;
        }
        outAnimation.steps.append(step);
    }

    return true;
}
} //namespace

bool AnimePluginLoader::LoadFromFile(const QString &filePath,
                                     AnimePluginDefinition &outPlugin,
                                     QString &outError)
{
    ZcJsonLib pluginConfig(filePath);
    if (!pluginConfig.load())
    {
        outError = QString("无法读取插件文件: %1").arg(filePath);
        return false;
    }

    outPlugin = AnimePluginDefinition();
    outPlugin.filePath = filePath;
    outPlugin.schema = pluginConfig.value("schema").toString().trimmed();
    outPlugin.pluginId = pluginConfig.value("pluginId").toString().trimmed();
    outPlugin.name = pluginConfig.value("name").toString().trimmed();
    outPlugin.author = pluginConfig.value("author").toString().trimmed();
    outPlugin.link = pluginConfig.value("link").toString().trimmed();

    if (outPlugin.schema != kSchemaV1)
    {
        outError = QString("schema 不匹配，当前仅支持 %1").arg(kSchemaV1);
        return false;
    }

    //插件元信息必须完整，供后续导入展示与索引使用
    if (outPlugin.pluginId.isEmpty() || outPlugin.name.isEmpty() ||
        outPlugin.author.isEmpty() || outPlugin.link.isEmpty())
    {
        outError = "插件必须包含 pluginId/name/author/link";
        return false;
    }

    const QJsonArray animations = pluginConfig.value("animations").toArray();
    if (animations.isEmpty())
    {
        outError = "animations 不能为空";
        return false;
    }

    QSet<QString> animationIdSet;
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

        //同一插件内动画id必须唯一
        if (animationIdSet.contains(animation.id))
        {
            outError = QString("动画 id 重复: %1").arg(animation.id);
            return false;
        }

        animationIdSet.insert(animation.id);
        outPlugin.animations.append(animation);
    }

    return true;
}

QStringList AnimePluginLoader::BuildAnimationDisplayNames(
    const AnimePluginDefinition &plugin)
{
    QStringList names;
    names.reserve(plugin.animations.size());
    for (const AnimePluginAnimation &animation : plugin.animations)
        names.append(animation.BuildDisplayName(plugin.name));
    return names;
}
