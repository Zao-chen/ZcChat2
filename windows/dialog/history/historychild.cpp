#include "historychild.h"
#include "ui_historychild.h"

historychild::historychild(int historyIndex, const QString &name, const QString &msg,
                           QWidget *parent)
    : QWidget(parent), ui(new Ui::historychild), m_historyIndex(historyIndex)
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

/*请求回溯*/
void historychild::on_pushButton_jump_clicked()
{
    emit jumpRequested(m_historyIndex);
}
