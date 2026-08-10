#include "ModelContextCatalog.h"

#include <QJsonObject>
#include <QStringList>

namespace
{
QString providerPrefix(const QString &provider)
{
    const QString normalizedProvider = provider.trimmed();
    if (normalizedProvider.compare(QStringLiteral("OpenAI"),
                                   Qt::CaseInsensitive) == 0)
        return QStringLiteral("openai");
    if (normalizedProvider.compare(QStringLiteral("DeepSeek"),
                                   Qt::CaseInsensitive) == 0)
        return QStringLiteral("deepseek");
    return {};
}

int contextTokenLimit(const QJsonValue &entry)
{
    return entry.toObject()
        .value(QStringLiteral("limit"))
        .toObject()
        .value(QStringLiteral("context"))
        .toInt();
}

ModelContextCatalog::Match matchEntry(const QJsonObject &catalog,
                                      const QString &requestedId)
{
    for (auto iterator = catalog.constBegin(); iterator != catalog.constEnd();
         ++iterator)
    {
        if (iterator.key().compare(requestedId, Qt::CaseInsensitive) != 0)
            continue;

        const int limit = contextTokenLimit(iterator.value());
        if (limit <= 0)
            return {};
        return {iterator.key(), limit};
    }
    return {};
}
} // namespace

namespace ModelContextCatalog
{
bool selectionMatches(const QString &storedProvider,
                      const QString &storedModel,
                      const QString &currentProvider,
                      const QString &currentModel)
{
    const QString normalizedModel = currentModel.trimmed();
    if (normalizedModel.isEmpty() ||
        storedModel.trimmed().compare(normalizedModel,
                                      Qt::CaseInsensitive) != 0)
        return false;

    const QString normalizedStoredProvider = storedProvider.trimmed();
    return normalizedStoredProvider.isEmpty() ||
           normalizedStoredProvider.compare(currentProvider.trimmed(),
                                            Qt::CaseInsensitive) == 0;
}

Match findContextWindow(const QJsonObject &catalog,
                        const QString &provider,
                        const QString &modelId)
{
    const QString trimmedModelId = modelId.trimmed();
    if (catalog.isEmpty() || trimmedModelId.isEmpty())
        return {};
    const QString prefix = providerPrefix(provider);
    const int firstSlash = trimmedModelId.indexOf('/');
    const int lastSlash = trimmedModelId.lastIndexOf('/');
    if (!prefix.isEmpty() && firstSlash >= 0 &&
        trimmedModelId.left(firstSlash)
                .compare(prefix, Qt::CaseInsensitive) != 0)
        return {};

    QStringList exactCandidates;
    if (!prefix.isEmpty() && lastSlash < 0)
        exactCandidates.append(prefix + QLatin1Char('/') + trimmedModelId);
    else
        exactCandidates.append(trimmedModelId);

    // OpenRouter 等服务有时在模型 ID 末尾追加 :free 之类的路由后缀。
    const int variantSeparator = trimmedModelId.lastIndexOf(':');
    if (variantSeparator > lastSlash)
    {
        const QString withoutVariant = trimmedModelId.left(variantSeparator);
        if (!prefix.isEmpty() && !withoutVariant.contains('/'))
            exactCandidates.append(prefix + QLatin1Char('/') + withoutVariant);
        else
            exactCandidates.append(withoutVariant);
    }

    exactCandidates.removeDuplicates();
    for (const QString &candidate : exactCandidates)
    {
        const Match match = matchEntry(catalog, candidate);
        if (match.isValid())
            return match;
    }

    // 内置服务商只接受准确归属，避免把同名模型误匹配到其他厂商。
    // 自定义 OpenAI 兼容服务可能只返回裸模型名；仅在结果唯一时采用后缀匹配。
    if (!prefix.isEmpty())
        return {};

    QString suffixModelId = trimmedModelId;
    if (variantSeparator > lastSlash)
        suffixModelId = trimmedModelId.left(variantSeparator);
    if (suffixModelId.contains('/'))
        return {};

    Match suffixMatch;
    const QString suffix = QLatin1Char('/') + suffixModelId;
    for (auto iterator = catalog.constBegin(); iterator != catalog.constEnd();
         ++iterator)
    {
        if (!iterator.key().endsWith(suffix, Qt::CaseInsensitive))
            continue;

        const int limit = contextTokenLimit(iterator.value());
        if (limit <= 0)
            continue;
        if (suffixMatch.isValid())
            return {};
        suffixMatch = {iterator.key(), limit};
    }

    return suffixMatch;
}
} // namespace ModelContextCatalog
