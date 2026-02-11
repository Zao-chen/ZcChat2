#ifndef SETTINGCHILD_LLM_H
#define SETTINGCHILD_LLM_H

#include <QWidget>

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

private:
    Ui::SettingChild_LLM *ui;
};

#endif // SETTINGCHILD_LLM_H
