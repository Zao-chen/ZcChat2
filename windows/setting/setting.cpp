#include "setting.h"
#include "./ui_setting.h"

#include "child/settingchild_llm.h"

MainWindow::MainWindow(QWidget *parent)
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
    addPageNode("大模型设置",settingchild_llmWin, ElaIconType::UserRobot);

}

MainWindow::~MainWindow()
{
    delete ui;
}
