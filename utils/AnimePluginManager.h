#pragma once

#include "AnimePlugin.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class AnimePluginManager
{
  public:
    bool Reload();

    const QList<AnimePluginDefinition> &Plugins() const;
    const QStringList &AnimationDisplayNames() const;
    const QStringList &AnimationUniqueKeys() const;
    const QStringList &LastErrors() const;

    bool TryGetAnimationByUniqueKey(const QString &uniqueKey,
                                    AnimePluginDefinition &outPlugin,
                                    AnimePluginAnimation &outAnimation) const;

  private:
    struct AnimationIndex
    {
        int pluginIndex = -1;
        int animationIndex = -1;
    };

    //key:pluginId:animationId,value:插件和动画在列表中的索引
    QHash<QString, AnimationIndex> m_animationIndexByUniqueKey;

    QList<AnimePluginDefinition> m_plugins;
    QStringList m_animationDisplayNames;
    QStringList m_animationUniqueKeys;
    QStringList m_lastErrors;
};
