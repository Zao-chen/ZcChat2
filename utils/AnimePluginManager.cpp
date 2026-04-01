#include "AnimePluginManager.h"

#include "GlobalConstants.h"

#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSet>

bool AnimePluginManager::Reload()
{
    //每次重载都清空缓存，避免旧索引残留
    m_plugins.clear();
    m_animationDisplayNames.clear();
    m_animationUniqueKeys.clear();
    m_animationIndexByUniqueKey.clear();
    m_lastErrors.clear();

    if (!EnsureAnimePluginDir())
    {
        m_lastErrors.append(QString("无法创建动画插件目录: %1").arg(AnimePluginPath));
        return false;
    }

    QDir pluginDir(AnimePluginPath);
    //按文件名排序，保证列表与UI展示稳定
    const QFileInfoList pluginFiles =
        pluginDir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);

    QSet<QString> pluginNameSet;
    for (const QFileInfo &pluginFile : pluginFiles)
    {
        AnimePluginDefinition plugin;
        QString error;
        if (!AnimePluginLoader::LoadFromFile(pluginFile.filePath(), plugin, error))
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

        //建立动画展示名与唯一键索引，供设置页和反查使用
        const AnimePluginDefinition &loadedPlugin = m_plugins.at(pluginIndex);
        for (int i = 0; i < loadedPlugin.animations.size(); ++i)
        {
            const AnimePluginAnimation &animation = loadedPlugin.animations.at(i);
            const QString uniqueKey = animation.BuildUniqueKey(loadedPlugin.name);
            const QString displayName = animation.BuildDisplayName(loadedPlugin.name);

            if (m_animationIndexByUniqueKey.contains(uniqueKey))
            {
                m_lastErrors.append(QString("动画唯一键冲突[%1]: %2")
                                        .arg(pluginFile.fileName())
                                        .arg(uniqueKey));
                continue;
            }

            m_animationUniqueKeys.append(uniqueKey);
            m_animationDisplayNames.append(displayName);
            m_animationIndexByUniqueKey.insert(uniqueKey, AnimationIndex{pluginIndex, i});
        }
    }

    //只要至少加载了一个插件就视为成功
    return !m_plugins.isEmpty();
}

const QList<AnimePluginDefinition> &AnimePluginManager::Plugins() const
{
    return m_plugins;
}

const QStringList &AnimePluginManager::AnimationDisplayNames() const
{
    return m_animationDisplayNames;
}

const QStringList &AnimePluginManager::AnimationUniqueKeys() const
{
    return m_animationUniqueKeys;
}

const QStringList &AnimePluginManager::LastErrors() const
{
    return m_lastErrors;
}

bool AnimePluginManager::TryGetAnimationByUniqueKey(
    const QString &uniqueKey, AnimePluginDefinition &outPlugin,
    AnimePluginAnimation &outAnimation) const
{
    //先查哈希索引，再做边界保护，避免脏数据越界
    if (!m_animationIndexByUniqueKey.contains(uniqueKey))
        return false;

    const AnimationIndex index = m_animationIndexByUniqueKey.value(uniqueKey);
    if (index.pluginIndex < 0 || index.pluginIndex >= m_plugins.size())
        return false;

    const AnimePluginDefinition &plugin = m_plugins.at(index.pluginIndex);
    if (index.animationIndex < 0 || index.animationIndex >= plugin.animations.size())
        return false;

    outPlugin = plugin;
    outAnimation = plugin.animations.at(index.animationIndex);
    return true;
}
