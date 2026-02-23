#include "settingchild_llm.h"
#include "ui_settingchild_llm.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"
#include <QSettings>
#include <QJsonArray>
#include <QJsonValue>

SettingChild_LLM::SettingChild_LLM(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_LLM)
{
    ui->setupUi(this);
    /*初始化*/
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("对话模型设置");
    modelListModel = new QStringListModel(this);
    ui->listView_ModelList->setModel(modelListModel);
    ai = new AiProvider(this);

    // 错误处理
    connect(ai, &AiProvider::errorOccurred, [=](const QString &error) {
        qWarning()<<error;
    });

}

SettingChild_LLM::~SettingChild_LLM()
{
    delete ui;
}

/*下一级菜单*/
void SettingChild_LLM::on_pushButton_Openai_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    NowSelectServer = "OpenAI";
    ui->BreadcrumbBar->appendBreadcrumb(NowSelectServer);

    /*读取配置*/
    //apikey
    ZcJsonLib config(JsonSettingPath);
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();
    ui->lineEdit_ApiKey->setText(apiKey);
    modelListModel->setStringList(QStringList());
    //模型列表
     QJsonArray modelIds = config.value("llm/" + NowSelectServer + "/ModelList").toArray();
     QStringList modelList;
     for (const auto &modelId : modelIds)
     {
         modelList.append(modelId.toString());
     }
     modelListModel->setStringList(modelList);
}
void SettingChild_LLM::on_pushButton_Deepseek_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    NowSelectServer = "DeepSeek";
    ui->BreadcrumbBar->appendBreadcrumb(NowSelectServer);

    //读取配置
    ZcJsonLib config(JsonSettingPath);
    QString apiKey = config.value("llm/" + NowSelectServer + "/ApiKey").toString();

    ui->lineEdit_ApiKey->setText(apiKey);
    modelListModel->setStringList(QStringList());
}

/*面包屑返回上级*/
void SettingChild_LLM::on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList)
{
    ui->stackedWidget->setCurrentIndex(0);
}

/*读取模型列表*/
void SettingChild_LLM::on_pushButton_LoadModelList_clicked()
{
    /*模型判断和初始化*/
    if(NowSelectServer == "OpenAI") ai->setServiceType(AiProvider::OpenAI);
    else if(NowSelectServer == "DeepSeek") ai->setServiceType(AiProvider::DeepSeek);
    ai->setApiKey(ui->lineEdit_ApiKey->text());
    ai->fetchModels();

    /*接收模型列表*/
    connect(ai, &AiProvider::modelsReceived, this,[=](const QList<AiProvider::ModelInfo> &models)
    {
        QStringList list;
        for (const auto &model : models)
        {
            QString displayText = model.id;
            if (!model.ownedBy.isEmpty()) displayText += QString(" (%1)").arg(model.ownedBy);
            list << displayText;
        }
        modelListModel->setStringList(list);

        //保存列表
        ZcJsonLib config(JsonSettingPath);
        QJsonArray modelIds;
        for(const auto &model : models)
        {
            modelIds.append(model.id);
        }
        config.setValue("llm/" + NowSelectServer + "/ModelList", QJsonValue(modelIds));
    });

}

/*配置修改*/
void SettingChild_LLM::on_lineEdit_ApiKey_textChanged(const QString &arg1)
{
    ZcJsonLib config(JsonSettingPath);
    config.setValue("llm/" + NowSelectServer + "/ApiKey", arg1);
}

