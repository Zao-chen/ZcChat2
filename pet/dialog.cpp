#include "dialog.h"
#include "ui_dialog.h"

#include "../GlobalConstants.h"

#include <QSettings>

Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //配置文件
    QSettings settings(Settingpath, QSettings::IniFormat);

    /*初始化AI*/
    ai = new AiProvider(this);
    ai->setServiceType(AiProvider::DeepSeek);
    ai->setApiKey(settings.value("llm/deepseek/key").toString());  // 替换成你的 Key
    //接收回复
    connect(ai, &AiProvider::replyReceived, [=](const QString &reply)
    {
        qDebug()<<reply;
    });
    //错误处理
    connect(ai, &AiProvider::errorOccurred, [=](const QString &error)
    {
        qDebug()<<error;
    });
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_SendTest_clicked()
{
    qDebug()<<"发送";
    ai->chat("test");
}

