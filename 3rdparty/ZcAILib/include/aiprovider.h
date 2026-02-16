#ifndef AIPROVIDER_H
#define AIPROVIDER_H

#include "ZcAiLib_global.h"
#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

class ZCAILIB_EXPORT AiProvider : public QObject
{
    Q_OBJECT

public:
    enum ServiceType {
        OpenAI,
        DeepSeek,
        Custom
    };

    // 模型信息
    struct ModelInfo {
        QString id;              // 模型 ID
        QString created;         // 创建时间
        QString ownedBy;         // 拥有者
        QStringList permissions; // 权限
    };

    explicit AiProvider(QObject *parent = nullptr);
    ~AiProvider();

    void setServiceType(ServiceType type);
    void setApiKey(const QString &apiKey);
    void setApiUrl(const QString &url);
    void setModel(const QString &model);

    QString currentModel() const { return m_model; }
    ServiceType currentServiceType() const { return m_serviceType; }

    // 从 API 获取模型列表
    void fetchModels();

    // 发送消息
    void chat(const QString &message);

    void setSystemPrompt(const QString &prompt);

    QString m_systemPrompt; // 系统提示词

signals:
    void replyReceived(const QString &reply);
    void errorOccurred(const QString &error);
    void modelsReceived(const QList<ModelInfo> &models);

private slots:
    void handleReply();
    void handleModelsReply();

private:
    QNetworkAccessManager *m_network;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
    QString m_modelsApiUrl;  // 专门用于获取模型列表的 URL
    ServiceType m_serviceType;
};

#endif // AIPROVIDER_H
