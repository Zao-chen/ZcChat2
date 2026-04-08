#ifndef SETTINGCHILD_CHAR_H
#define SETTINGCHILD_CHAR_H

#include "../../../utils/AnimePluginManager.h"

#include <QWidget>

#include <QList>

namespace Ui
{
class SettingChild_Char;
}

class SettingChild_Char : public QWidget
{
    Q_OBJECT

  public:
    explicit SettingChild_Char(QWidget *parent = nullptr);
    ~SettingChild_Char();
    //公开刷新方法供外部调用
    void RefreshModelList();     ////刷新LLM模型列表
    void RefreshVitsModelList(); ////刷新Vits模型列表

  private slots:
    void on_pushButton_DeleteChar_clicked();
    void on_comboBox_CharList_currentTextChanged(const QString &arg1);
    void on_plainTextEdit_CharPrompt_textChanged();
    void on_spinBox_TachieSize_textChanged(const QString &arg1);
    void on_comboBox_ServerSelect_currentTextChanged(const QString &arg1);
    void on_comboBox_ModelSelect_currentTextChanged(const QString &arg1);
    void on_pushButton_ResetTachieLoc_clicked();
    void on_comboBox_Vits_MASSelect_currentTextChanged(const QString &arg1);
    void on_ToggleSwitch_VitsEnable_toggled(bool checked);
    void on_pushButton_InputChar_clicked();
    void on_pushButton_OutputChar_clicked();
    void on_pushButton_Tachie_Set_clicked();
    void on_BreadcrumbBar_breadcrumbClicked(QString breadcrumb,
                                            QStringList lastBreadcrumbList);

  signals:
    void requestReloadCharSelect(QString TachieName);
    void requestSetTachieSize(int size);
    void requestResetTachieLoc();
    void requestReloadAIConfig();

  private:
    Ui::SettingChild_Char *ui;
    bool isAlreadyLoading = false;
    AnimePluginManager m_pluginManager;
    QList<QWidget *> m_tachieBindingRows;
    void LoadCurrentCharConfig();
    void RefreshCharList();
    void ClearTachieBindingRows();
    void RefreshTachieActionList();
    void RefreshTachieAnimationList();
};

#endif //SETTINGCHILD_CHAR_H
