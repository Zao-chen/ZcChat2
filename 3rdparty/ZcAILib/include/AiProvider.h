#ifndef AIPROVIDER_H
#define AIPROVIDER_H

#include "ZcAiLib_global.h"
#include <QByteArray>
#include <QHash>
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

    struct ModelInfo {
        QString id;
        QString created;
        QString ownedBy;
        QStringList permissions;
    };

    explicit AiProvider(QObject *parent = nullptr);
    ~AiProvider();

    void setServiceType(ServiceType type);
    void setApiKey(const QString &apiKey);
    void setApiUrl(const QString &url);
    void setModel(const QString &model);
    void setStreamEnabled(bool enabled);

    QString currentModel() const { return m_model; }
    ServiceType currentServiceType() const { return m_serviceType; }
    bool isStreamEnabled() const { return m_streamEnabled; }

    void fetchModels();
    void chat(const QString &message);
    void setSystemPrompt(const QString &prompt);

    QString m_systemPrompt;

signals:
    void replyReceived(const QString &reply);
    void replyChunkReceived(const QString &chunk);
    void errorOccurred(const QString &error);
    void modelsReceived(const QList<ModelInfo> &models);

private slots:
    void handleReply();
    void handleStreamReadyRead();
    void handleModelsReply();

private:
    void processStreamChunk(QNetworkReply *reply, const QByteArray &chunk);
    void finalizeStreamReply(QNetworkReply *reply);
    void cleanupStreamReply(QNetworkReply *reply);

    QNetworkAccessManager *m_network;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_model;
    bool m_streamEnabled;
    QString m_modelsApiUrl;
    ServiceType m_serviceType;
    QHash<QNetworkReply *, QByteArray> m_streamBuffers;
    QHash<QNetworkReply *, QByteArray> m_rawResponses;
    QHash<QNetworkReply *, QString> m_streamReplies;
};

#endif // AIPROVIDER_H
