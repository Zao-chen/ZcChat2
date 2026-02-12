#ifndef SETTINGCHILD_LLM_H
#define SETTINGCHILD_LLM_H

#include "AiProvider.h"
#include <QWidget>
#include <QStringListModel>

namespace Ui {
class SettingChild_LLM;
}

class SettingChild_LLM : public QWidget
{
    Q_OBJECT

public:
    explicit SettingChild_LLM(QWidget *parent = nullptr);
    ~SettingChild_LLM();

private slots:
    void on_pushButton_Openai_Set_clicked();
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList);
    void on_pushButton_LoadModelList_clicked();
    void on_pushButton_Deepseek_Set_clicked();
    void on_lineEdit_ApiKey_textChanged(const QString &arg1);

private:
    Ui::SettingChild_LLM *ui;
    AiProvider *ai; //用于AI交互
    QString NowSelectServer; //当前正在编辑的模型
    QStringListModel *modelListModel;
};

#endif // SETTINGCHILD_LLM_H
