#include "settingchild_vits.h"
#include "ui_settingchild_vits.h"

#include "../../../GlobalConstants.h"

#include "ZcJsonLib.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringListModel>

SettingChild_Vits::SettingChild_Vits(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_Vits)
{
    ui->setupUi(this);
    /*初始化*/
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("语音合成设置");
}

SettingChild_Vits::~SettingChild_Vits()
{
    delete ui;
}

void SettingChild_Vits::on_lineEdit_ApiUrl_textChanged(const QString &arg1)
{
    /*保存配置*/
    ZcJsonLib config(JsonSettingPath);
    config.setValue("vits/ApiUrl", arg1);
}

/*VSA设置*/
void SettingChild_Vits::on_pushButton_VSA_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("vits-simple-api");
    /*读取*/
    //读取key
    ZcJsonLib config(JsonSettingPath);
    QString apiUrl = config.value("vits/ApiUrl").toString();
    ui->lineEdit_ApiUrl->setText(apiUrl);
    //读取模型角色列表
    QStringList list;
    QJsonArray arr = config.value("vits/ModelAndSpeakerList").toArray();
    for (const QJsonValue& val : arr)  list.append(val.toString());
    QStringListModel* model = new QStringListModel(list,ui->listView_ModelAndSpeakerlList);
    ui->listView_ModelAndSpeakerlList->setModel(model);
}

/*面包屑返回上级*/
void SettingChild_Vits::on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList)
{
    ui->stackedWidget->setCurrentIndex(0);
}

/*读取角色和模型列表*/
void SettingChild_Vits::on_pushButton_LoadModelAndSpeakerlList_clicked()
{
    //发送get请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QUrl url(ui->lineEdit_ApiUrl->text() + "/voice/speakers"); //改成你的地址
    QNetworkRequest request(url);
    QNetworkReply* reply = manager->get(request);
    connect(reply,&QNetworkReply::finished,this,[=]()
    {
        if(reply->error() != QNetworkReply::NoError)
        {
            reply->deleteLater();
            manager->deleteLater();
            return;
        }
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if(!doc.isObject())
        {
            reply->deleteLater();
            manager->deleteLater();
            return;
        }
        QStringList list;
        QJsonObject rootObj = doc.object();
        for(auto it = rootObj.begin(); it != rootObj.end(); ++it)
        {
            QString modelType = it.key();
            QJsonArray arr = it.value().toArray();

            for(const QJsonValue& val : arr)
            {
                QJsonObject obj = val.toObject();
                QString name = obj.value("name").toString();
                int id = obj.value("id").toInt();

                QString displayText = modelType + " - " + QString::number(id) + " - " + name;
                list.append(displayText);
            }
        }
        QStringListModel* model = new QStringListModel(list,ui->listView_ModelAndSpeakerlList);
        ui->listView_ModelAndSpeakerlList->setModel(model);
        reply->deleteLater();
        manager->deleteLater();
        //保存
        ZcJsonLib config(JsonSettingPath);
        QJsonArray arr;
        for(const QString& s : list)    arr.append(s);
        config.setValue("vits/ModelAndSpeakerList", arr);
    });
}
