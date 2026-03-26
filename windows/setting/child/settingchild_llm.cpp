#include "settingchild_llm.h"
#include "ui_settingchild_llm.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include <QJsonArray>
#include <QJsonValue>
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

    //读取配置
    ZcJsonLib config(JsonSettingPath);
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();

    isLoadingConfig = true;
    ui->lineEdit_ApiKey->setText(apiKey);
    isLoadingConfig = false;
    modelListModel->setStringList(QStringList());
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
}

/*读取模型列表*/
void SettingChild_LLM::on_pushButton_LoadModelList_clicked()
{
    if (NowSelectServer == "OpenAI")
        ai->setServiceType(AiProvider::OpenAI);
    else if (NowSelectServer == "DeepSeek")
        ai->setServiceType(AiProvider::DeepSeek);

    ai->setApiKey(ui->lineEdit_ApiKey->text());
    modelFetchServer = NowSelectServer;
    ai->fetchModels();
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
}
