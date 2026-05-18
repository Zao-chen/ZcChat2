#include "settingchild_llm.h"
#include "ui_settingchild_llm.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>

SettingChild_LLM::SettingChild_LLM(QWidget *parent)
    : QWidget(parent), ui(new Ui::SettingChild_LLM)
{
    ui->setupUi(this);
    /*初始化*/
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("对话模型设置");
    modelListModel = new QStringListModel(this);
    ui->listView_ModelList->setModel(modelListModel);
    ai = new AiProvider(this);
    customNetwork = new QNetworkAccessManager(this);

    //错误处理
    connect(ai, &AiProvider::errorOccurred, [=](const QString &error)
            {
                qWarning() << error;
                modelFetchServer.clear(); });
    //接收模型列表
    connect(ai, &AiProvider::modelsReceived, this,
            [this](const QList<AiProvider::ModelInfo> &models)
            {
                const QString fetchedServer = modelFetchServer;
                modelFetchServer.clear();
                //获取模型列表
                QStringList list;
                QJsonArray modelIds;
                for (const AiProvider::ModelInfo &model : models)
                {
                    QString displayText = model.id;
                    if (!model.ownedBy.isEmpty())
                        displayText += QString(" (%1)").arg(model.ownedBy);

                    list << displayText;
                    modelIds.append(model.id);
                }
                //列表保存
                if (!fetchedServer.isEmpty())
                {
                    ZcJsonLib config(JsonSettingPath);
                    config.setValue("llm/" + fetchedServer + "/ModelList",
                                    QJsonValue(modelIds));
                    //状态更新
                    if (fetchedServer == "OpenAI")
                        ui->label_Openai_Status->setVisible(true);
                    else if (fetchedServer == "DeepSeek")
                        ui->label_Deepseek_Status->setVisible(true);
                    else if (fetchedServer == "Custom")
                        ui->label_Custom_Status->setVisible(true);
                    //发出模型列表刷新信号
                    emit modelListRefreshed();
                }

                if (NowSelectServer == fetchedServer)
                    modelListModel->setStringList(list);
            });
    //默认读取状态
    ZcJsonLib config(JsonSettingPath);
    QString openAiApiKey =
        config.value("llm/OpenAI/ApiKey").toString().trimmed();
    QJsonArray openAiModelIds =
        config.value("llm/OpenAI/ModelList").toArray();
    ui->label_Openai_Status->setVisible(
        !openAiApiKey.isEmpty() && !openAiModelIds.isEmpty());

    QString deepSeekApiKey =
        config.value("llm/DeepSeek/ApiKey").toString().trimmed();
    QJsonArray deepSeekModelIds =
        config.value("llm/DeepSeek/ModelList").toArray();
    ui->label_Deepseek_Status->setVisible(
        !deepSeekApiKey.isEmpty() && !deepSeekModelIds.isEmpty());

    QString customBaseUrl =
        config.value("llm/Custom/BaseUrl").toString().trimmed();
    QString customApiKey =
        config.value("llm/Custom/ApiKey").toString().trimmed();
    QJsonArray customModelIds =
        config.value("llm/Custom/ModelList").toArray();
    ui->label_Custom_Status->setVisible(
        !customBaseUrl.isEmpty() && !customApiKey.isEmpty() && !customModelIds.isEmpty());
}

SettingChild_LLM::~SettingChild_LLM()
{
    delete ui;
}

/*下一级菜单*/
//OpenAI
void SettingChild_LLM::on_pushButton_Openai_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    NowSelectServer = "OpenAI";
    ui->BreadcrumbBar->appendBreadcrumb(NowSelectServer);
    ui->widget_BaseUrl->setVisible(false);

    /*读取配置*/
    //apikey
    ZcJsonLib config(JsonSettingPath);
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();
    isLoadingConfig = true;
    ui->lineEdit_ApiKey->setText(apiKey);
    isLoadingConfig = false;
    modelListModel->setStringList(QStringList());
    //模型列表
    QJsonArray modelIds = config.value("llm/" + NowSelectServer + "/ModelList").toArray();
    QStringList modelList;
    for (const QJsonValue &modelId : modelIds)
    {
        modelList.append(modelId.toString());
    }
    modelListModel->setStringList(modelList);
}
//DeepSeek
void SettingChild_LLM::on_pushButton_Deepseek_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    NowSelectServer = "DeepSeek";
    ui->BreadcrumbBar->appendBreadcrumb(NowSelectServer);
    ui->widget_BaseUrl->setVisible(false);

    //读取配置
    ZcJsonLib config(JsonSettingPath);
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();
    isLoadingConfig = true;
    ui->lineEdit_ApiKey->setText(apiKey);
    isLoadingConfig = false;
    modelListModel->setStringList(QStringList());
    //模型列表
    QJsonArray modelIds = config.value("llm/" + NowSelectServer + "/ModelList").toArray();
    QStringList modelList;
    for (const QJsonValue &modelId : modelIds)
    {
        modelList.append(modelId.toString());
    }
    modelListModel->setStringList(modelList);
}
//Custom
void SettingChild_LLM::on_pushButton_Custom_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    NowSelectServer = "Custom";
    ui->BreadcrumbBar->appendBreadcrumb(NowSelectServer);
    ui->widget_BaseUrl->setVisible(true);

    //读取配置
    ZcJsonLib config(JsonSettingPath);
    QString baseUrl = config.value("llm/" + NowSelectServer + "/BaseUrl").toString();
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();
    isLoadingConfig = true;
    ui->lineEdit_BaseUrl->setText(baseUrl);
    ui->lineEdit_ApiKey->setText(apiKey);
    isLoadingConfig = false;
    modelListModel->setStringList(QStringList());
    //模型列表
    QJsonArray modelIds = config.value("llm/" + NowSelectServer + "/ModelList").toArray();
    QStringList modelList;
    for (const QJsonValue &modelId : modelIds)
    {
        modelList.append(modelId.toString());
    }
    modelListModel->setStringList(modelList);
}

/*面包屑返回上级*/
void SettingChild_LLM::on_BreadcrumbBar_breadcrumbClicked(
    QString breadcrumb, QStringList lastBreadcrumbList)
{
    Q_UNUSED(breadcrumb);
    Q_UNUSED(lastBreadcrumbList);

    ui->stackedWidget->setCurrentIndex(0);

    //状态更新
    ZcJsonLib config(JsonSettingPath);
    QString openAiApiKey =
        config.value("llm/OpenAI/ApiKey").toString().trimmed();
    QJsonArray openAiModelIds =
        config.value("llm/OpenAI/ModelList").toArray();
    ui->label_Openai_Status->setVisible(
        !openAiApiKey.isEmpty() && !openAiModelIds.isEmpty());

    QString deepSeekApiKey =
        config.value("llm/DeepSeek/ApiKey").toString().trimmed();
    QJsonArray deepSeekModelIds =
        config.value("llm/DeepSeek/ModelList").toArray();
    ui->label_Deepseek_Status->setVisible(
        !deepSeekApiKey.isEmpty() && !deepSeekModelIds.isEmpty());

    QString customBaseUrl =
        config.value("llm/Custom/BaseUrl").toString().trimmed();
    QString customApiKey =
        config.value("llm/Custom/ApiKey").toString().trimmed();
    QJsonArray customModelIds =
        config.value("llm/Custom/ModelList").toArray();
    ui->label_Custom_Status->setVisible(
        !customBaseUrl.isEmpty() && !customApiKey.isEmpty() && !customModelIds.isEmpty());
}

/*读取模型列表*/
void SettingChild_LLM::on_pushButton_LoadModelList_clicked()
{
    if (NowSelectServer == "OpenAI")
    {
        ai->setServiceType(AiProvider::OpenAI);
        ai->setApiKey(ui->lineEdit_ApiKey->text());
        modelFetchServer = NowSelectServer;
        ai->fetchModels();
    }
    else if (NowSelectServer == "DeepSeek")
    {
        ai->setServiceType(AiProvider::DeepSeek);
        ai->setApiKey(ui->lineEdit_ApiKey->text());
        modelFetchServer = NowSelectServer;
        ai->fetchModels();
    }
    else if (NowSelectServer == "Custom")
    {
        fetchCustomModels(ui->lineEdit_BaseUrl->text().trimmed(),
                          ui->lineEdit_ApiKey->text());
    }
}

/*自定义提供商模型拉取*/
void SettingChild_LLM::fetchCustomModels(const QString &baseUrl,
                                         const QString &apiKey)
{
    if (baseUrl.isEmpty() || apiKey.isEmpty())
    {
        qWarning() << "Custom provider: Base URL or API Key is empty";
        return;
    }

    //构建模型列表URL
    QString modelsUrl = baseUrl;
    if (modelsUrl.endsWith('/'))
        modelsUrl.chop(1);
    if (!modelsUrl.endsWith("/models"))
        modelsUrl += "/models";

    QNetworkRequest request{QUrl(modelsUrl)};
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    modelFetchServer = "Custom";
    QNetworkReply *reply = customNetwork->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
            {
                reply->deleteLater();
                if (reply->error() != QNetworkReply::NoError)
                {
                    qWarning() << "Custom fetchModels failed:" << reply->errorString();
                    modelFetchServer.clear();
                    return;
                }

                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (!doc.isObject() || !doc.object().contains("data"))
                {
                    qWarning() << "Custom fetchModels: invalid response";
                    modelFetchServer.clear();
                    return;
                }

                QJsonArray items = doc.object().value("data").toArray();
                QStringList list;
                QJsonArray modelIds;
                for (const QJsonValue &val : items)
                {
                    QString id = val.toObject().value("id").toString();
                    if (!id.isEmpty())
                    {
                        list << id;
                        modelIds.append(id);
                    }
                }

                const QString fetchedServer = modelFetchServer;
                modelFetchServer.clear();

                if (!fetchedServer.isEmpty())
                {
                    ZcJsonLib config(JsonSettingPath);
                    config.setValue("llm/" + fetchedServer + "/ModelList",
                                    QJsonValue(modelIds));
                    ui->label_Custom_Status->setVisible(true);
                    emit modelListRefreshed();
                }

                if (NowSelectServer == fetchedServer)
                    modelListModel->setStringList(list);
            });
}

/*配置修改*/
void SettingChild_LLM::on_lineEdit_ApiKey_textChanged(const QString &arg1)
{
    if (NowSelectServer.isEmpty())
        return;

    ZcJsonLib config(JsonSettingPath);
    config.setValue("llm/" + NowSelectServer + "/ApiKey", arg1);

    if (isLoadingConfig)
        return;

    config.setValue("llm/" + NowSelectServer + "/ModelList", QJsonArray());
    modelListModel->setStringList(QStringList());

    //刷新状态为false
    if (NowSelectServer == "OpenAI")
        ui->label_Openai_Status->setVisible(false);
    else if (NowSelectServer == "DeepSeek")
        ui->label_Deepseek_Status->setVisible(false);
    else if (NowSelectServer == "Custom")
        ui->label_Custom_Status->setVisible(false);
}

/*Base URL修改*/
void SettingChild_LLM::on_lineEdit_BaseUrl_textChanged(const QString &arg1)
{
    if (NowSelectServer.isEmpty() || NowSelectServer != "Custom")
        return;

    ZcJsonLib config(JsonSettingPath);
    config.setValue("llm/Custom/BaseUrl", arg1);

    if (isLoadingConfig)
        return;

    config.setValue("llm/Custom/ModelList", QJsonArray());
    modelListModel->setStringList(QStringList());
    ui->label_Custom_Status->setVisible(false);
}
