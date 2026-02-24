#ifndef SETTINGCHILD_CHAR_H
#define SETTINGCHILD_CHAR_H

#include <QWidget>

namespace Ui {
class SettingChild_Char;
}

class SettingChild_Char : public QWidget
{
    Q_OBJECT

public:
    explicit SettingChild_Char(QWidget *parent = nullptr);
    ~SettingChild_Char();

private slots:
    void on_pushButton_RefreshCharList_clicked();
    void on_comboBox_CharList_currentTextChanged(const QString &arg1);
    void on_plainTextEdit_CharPrompt_textChanged();
    void on_spinBox_TachieSize_textChanged(const QString &arg1);
    void on_comboBox_ServerSelect_currentTextChanged(const QString &arg1);
    void on_comboBox_ModelSelect_currentTextChanged(const QString &arg1);

    void on_pushButton_ResetTachieLoc_clicked();

signals:
    void requestReloadCharSelect(QString TachieName);
    void requestSetTachieSize(int size);
    void requestResetTachieLoc();
    void requestReloadAIConfig();

private:
    Ui::SettingChild_Char *ui;
    void RefreshCharList();
    void RefreshModelList();
};

#endif // SETTINGCHILD_CHAR_H
