#include "AnimePluginManager.h"

#include "GlobalConstants.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSet>

/*重载插件*/
bool AnimePluginManager::Reload()
{
    //每次重载都清空缓存，避免旧索引残留
    m_plugins.clear();
    m_animationUniqueKeys.clear();
    m_animationIndexByUniqueKey.clear();
    m_lastErrors.clear();

    QDir pluginDir(AnimePluginPath);
    //按文件名排序，保证列表与UI展示稳定
    const QFileInfoList pluginFiles =
        pluginDir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);

    QSet<QString> pluginNameSet;

    //逐个加载插件文件
    for (const QFileInfo &pluginFile : pluginFiles)
    {
        AnimePluginDefinition plugin;
        QString error;

        //加载插件
        if (!LoadAnimePluginFromFile(pluginFile.filePath(), plugin, error))
        {
            m_lastErrors.append(
                QString("插件加载失败[%1]: %2").arg(pluginFile.fileName()).arg(error));
            continue;
        }

        //拒绝重复插件名，防止设置界面无法区分
        if (pluginNameSet.contains(plugin.name))
        {
            m_lastErrors.append(QString("插件冲突[%1]: 名称重复(%2)")
                                    .arg(pluginFile.fileName())
                                    .arg(plugin.name));
            continue;
        }

        const int pluginIndex = m_plugins.size();
        m_plugins.append(plugin);
        pluginNameSet.insert(plugin.name);

        //建立动画唯一键索引，即 插件名_动画名
        const AnimePluginDefinition &loadedPlugin = m_plugins.at(pluginIndex);
        //加载所有动画
        for (int i = 0; i < loadedPlugin.animations.size(); ++i)
        {
            const AnimePluginAnimation &animation = loadedPlugin.animations.at(i);
            const QString uniqueKey = animation.BuildUniqueKey(loadedPlugin.name);

            if (m_animationIndexByUniqueKey.contains(uniqueKey))
            {
                m_lastErrors.append(QString("动画唯一键冲突[%1]: %2")
                                        .arg(pluginFile.fileName())
                                        .arg(uniqueKey));
                continue;
            }

            m_animationUniqueKeys.append(uniqueKey);
            m_animationIndexByUniqueKey.insert(uniqueKey, PluginAndAnimationIndex{pluginIndex, i});
        }
    }

    //只要至少加载了一个插件就视为成功
    return !m_plugins.isEmpty();
}

/*获取插件列表*/
const QList<AnimePluginDefinition> &AnimePluginManager::Plugins() const
{
    return m_plugins;
}

/*获取所有动画唯一键列表，格式为 插件名_动画名*/
const QStringList &AnimePluginManager::AnimationUniqueKeys() const
{
    return m_animationUniqueKeys;
}

/*获取上次加载插件的错误列表*/
const QStringList &AnimePluginManager::LastErrors() const
{
    return m_lastErrors;
}

/*根据唯一键获取动画*/
bool AnimePluginManager::TryGetAnimationByUniqueKey(
    const QString &uniqueKey, AnimePluginDefinition &outPlugin,
    AnimePluginAnimation &outAnimation) const
{
    //先查哈希索引，再做边界保护，避免脏数据越界
    if (!m_animationIndexByUniqueKey.contains(uniqueKey))
        return false;

    const PluginAndAnimationIndex index = m_animationIndexByUniqueKey.value(uniqueKey);
    if (index.pluginIndex < 0 || index.pluginIndex >= m_plugins.size())
        return false;

    const AnimePluginDefinition &plugin = m_plugins.at(index.pluginIndex);
    if (index.animationIndex < 0 || index.animationIndex >= plugin.animations.size())
        return false;

    outPlugin = plugin;
    outAnimation = plugin.animations.at(index.animationIndex);
    return true;
}
