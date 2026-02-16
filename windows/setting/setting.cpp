#include "setting.h"
#include "./ui_setting.h"

#include "child/settingchild_llm.h"
#include "child/settingchild_char.h"

MainWindow::MainWindow(Dialog *dialog, Tachie *tachie, QWidget *parent)
    : ElaWindow(parent)
    , ui(new Ui::MainWindow)
{
    //ui->setupUi(this);

    /*初始化窗口*/
    setWindowTitle("ZcChat2");
    setUserInfoCardVisible(false);

    /*创建窗口*/
    SettingChild_LLM *settingchild_llmWin = new SettingChild_LLM(this);
    settingchild_llmWin->show();
    addPageNode("对话模型设置",settingchild_llmWin, ElaIconType::Message);
    SettingChild_Char *settingchild_charWin = new SettingChild_Char(this);
    settingchild_charWin->show();
    addPageNode("角色设置",settingchild_charWin, ElaIconType::SquareUser);

    //连接
    connect(settingchild_charWin, &SettingChild_Char::requestReloadCharSelect,
            tachie, &Tachie::SetCharTachie);

}

MainWindow::~MainWindow()
{
    delete ui;
}
