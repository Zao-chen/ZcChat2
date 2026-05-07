#include "setting.h"
#include "./ui_setting.h"

#include "child/settingchild_about.h"
#include "child/settingchild_char.h"
#include "child/settingchild_llm.h"
#include "child/settingchild_plugin.h"
#include "child/settingchild_speech.h"
#include "child/settingchild_vits.h"

MainWindow::MainWindow(Dialog *dialog, Tachie *tachie, QWidget *parent)
    : ElaWindow(parent), ui(new Ui::MainWindow)
{
    /*初始化窗口*/
    setWindowTitle("ZcChat2");
    setUserInfoCardVisible(false);

    /*创建窗口*/
    SettingChild_LLM *settingchild_llmWin = new SettingChild_LLM(this);
    settingchild_llmWin->show();
    addPageNode("对话模型", settingchild_llmWin, ElaIconType::Message);
    SettingChild_Speech *settingchild_speechWin = new SettingChild_Speech(this);
    settingchild_speechWin->show();
    addPageNode("语音输入", settingchild_speechWin, ElaIconType::Microphone);

    SettingChild_Vits *settingchild_vitsWin = new SettingChild_Vits(this);
    settingchild_vitsWin->show();
    addPageNode("语音合成", settingchild_vitsWin, ElaIconType::Bullhorn);

    SettingChild_Plugin *settingchild_pluginWin = new SettingChild_Plugin(this);
    settingchild_pluginWin->show();
    addPageNode("插件配置", settingchild_pluginWin, ElaIconType::PuzzlePiece);
    SettingChild_Char *settingchild_charWin = new SettingChild_Char(this);
    settingchild_charWin->show();
    addPageNode("角色设置", settingchild_charWin, ElaIconType::SquareUser);
    SettingChild_About *settingchild_aboutWin = new SettingChild_About(this);
    settingchild_aboutWin->show();
    QString settingchild_aboutKey = "about";
    addFooterNode("关于", settingchild_aboutWin, settingchild_aboutKey, 0, ElaIconType::CircleInfo);

    //连接
    connect(settingchild_charWin, &SettingChild_Char::requestReloadCharSelect,
            tachie, &Tachie::SetTachieImg); //设置立绘图像（重载角色）
    connect(settingchild_charWin, &SettingChild_Char::requestSetTachieSize,
            tachie, &Tachie::SetTachieSize); //设置立绘大小
    connect(settingchild_charWin, &SettingChild_Char::requestResetTachieLoc,
            tachie, &Tachie::ResetTachieLoc); //重置立绘位置
    connect(settingchild_charWin, &SettingChild_Char::requestReloadAIConfig,
            dialog, &Dialog::ReloadAIConfig); //重载AI配置
    //获取模型列表后刷新角色页的模型下拉框
    connect(settingchild_llmWin, &SettingChild_LLM::modelListRefreshed,
            settingchild_charWin, &SettingChild_Char::RefreshModelList); //刷新LLM模型列表
    connect(settingchild_vitsWin, &SettingChild_Vits::vitsModelListRefreshed,
            settingchild_charWin, &SettingChild_Char::RefreshVitsModelList); //刷新Vits模型列表
    connect(settingchild_speechWin, &SettingChild_Speech::speechConfigChanged,
            dialog, &Dialog::ReloadSpeechInputConfig); //刷新语音输入配置
}

MainWindow::~MainWindow()
{
    delete ui;
}
