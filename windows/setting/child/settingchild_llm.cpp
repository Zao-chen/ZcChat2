#include "settingchild_llm.h"
#include "ui_settingchild_llm.h"

SettingChild_LLM::SettingChild_LLM(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingChild_LLM)
{
    ui->setupUi(this);
    ui->BreadcrumbBar->setTextPixelSize(25);
    ui->BreadcrumbBar->appendBreadcrumb("大模型设置");

}

SettingChild_LLM::~SettingChild_LLM()
{
    delete ui;
}

/*OpenAI下一级菜单*/
void SettingChild_LLM::on_pushButton_Openai_Set_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
    ui->BreadcrumbBar->appendBreadcrumb("OpenAI");
}

/*面包屑*/
void SettingChild_LLM::on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList)
{
    ui->stackedWidget->setCurrentIndex(0);
}

