#include "historychild.h"
#include "ui_historychild.h"

historychild::historychild(const QString &name, const QString &msg,
                           QWidget *parent)
    : QWidget(parent), ui(new Ui::historychild)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
    //显示不抢占焦点
    setAttribute(Qt::WA_ShowWithoutActivating);
    //获取信息
    ui->label_name->setText(name);
    ui->label_msg->setText(msg);
    ui->pushButton_reSpawnVoice->hide();
}

historychild::~historychild()
{
    delete ui;
}
