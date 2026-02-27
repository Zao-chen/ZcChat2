#ifndef SETTINGCHILD_VITS_H
#define SETTINGCHILD_VITS_H

#include <QWidget>

namespace Ui {
class SettingChild_Vits;
}

class SettingChild_Vits : public QWidget
{
    Q_OBJECT

public:
    explicit SettingChild_Vits(QWidget *parent = nullptr);
    ~SettingChild_Vits();

private:
    Ui::SettingChild_Vits *ui;
};

#endif // SETTINGCHILD_VITS_H
