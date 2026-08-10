#ifndef SETTINGCHILD_LLM_H
#define SETTINGCHILD_LLM_H

#include "AiProvider.h"
#include <QStringListModel>
#include <QWidget>

namespace Ui
{
class SettingChild_LLM;
}

class QTimer;

class SettingChild_LLM : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_LLM(QWidget *parent = nullptr);
    ~SettingChild_LLM();

  signals:
    void modelListRefreshed(); //LLM模型列表已刷新
    void llmConfigChanged();   //API Key 或接口地址已修改

  private slots:
    void on_pushButton_Openai_Set_clicked();
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb, QStringList lastBreadcrumbList);
    void on_pushButton_LoadModelList_clicked();
    void on_pushButton_Deepseek_Set_clicked();
    void on_pushButton_Custom_Set_clicked();
    void on_lineEdit_ApiKey_textChanged(const QString &arg1);
    void on_lineEdit_BaseUrl_textChanged(const QString &arg1);

  private:
    Ui::SettingChild_LLM *ui;
    AiProvider *ai;          //用于AI交互
    QString NowSelectServer; //当前正在编辑的模型
    QString modelFetchServer;
    QStringListModel *modelListModel;
    QTimer *m_configReloadDebounce = nullptr;
    bool isLoadingConfig = false;
};

#endif //SETTINGCHILD_LLM_H
