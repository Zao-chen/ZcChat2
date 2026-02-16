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

signals:
    void requestReloadCharSelect(QString TachieName);

private:
    Ui::SettingChild_Char *ui;
    void RefreshCharList();
};

#endif // SETTINGCHILD_CHAR_H
