#ifndef MODELCONTEXTCATALOG_H
#define MODELCONTEXTCATALOG_H

#include <QJsonObject>
#include <QString>

namespace ModelContextCatalog
{
struct Match
{
    QString canonicalModelId;
    int contextTokenLimit = 0;

    bool isValid() const
    {
        return !canonicalModelId.isEmpty() && contextTokenLimit > 0;
    }
};

// 比较保存的模型选择；服务商为空时兼容旧配置。
bool selectionMatches(const QString &storedProvider,
                      const QString &storedModel,
                      const QString &currentProvider,
                      const QString &currentModel);

// 根据已解析的 models.dev 目录和实际请求模型 ID 查找上下文窗口。
Match findContextWindow(const QJsonObject &catalog,
                        const QString &provider,
                        const QString &modelId);
} // namespace ModelContextCatalog

#endif // MODELCONTEXTCATALOG_H
