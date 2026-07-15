#include "setting.h"

#include "child/settingchild_about.h"
#include "child/settingchild_char.h"
#include "child/settingchild_general.h"
#include "child/settingchild_llm.h"
#include "child/settingchild_plugin.h"
#include "child/settingchild_speech.h"
#include "child/settingchild_vits.h"

#include "ElaTheme.h"

#include <QAbstractScrollArea>
#include <QIcon>
#include <QPalette>
#include <QScrollArea>

namespace
{
/*应用Ela主题色到滚动区域及其内容*/
void applyScrollAreaTheme(QAbstractScrollArea *area,
                          ElaThemeType::ThemeMode themeMode)
{
    const QColor background =
        ElaThemeColor(themeMode, WindowCentralStackBase);
    const QColor text = ElaThemeColor(themeMode, BasicText);
    const QColor disabledText = ElaThemeColor(themeMode, BasicTextDisable);

    QPalette palette = area->palette();
    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::AlternateBase, background);
    palette.setColor(QPalette::Window, background);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    area->setPalette(palette);

    QWidget *viewport = area->viewport();
    if (viewport)
    {
        //viewport使用与滚动区域相同的主题色，避免出现原生白底
        viewport->setPalette(palette);
        viewport->setAutoFillBackground(true);
        viewport->update();
    }

    if (QScrollArea *scrollArea = qobject_cast<QScrollArea *>(area))
    {
        if (QWidget *content = scrollArea->widget())
        {
            //同步滚动区域内容容器的背景色
            content->setPalette(palette);
            content->setAutoFillBackground(true);
            content->update();
        }
    }

    area->update();
}

/*绑定设置页滚动控件的主题切换*/
void bindScrollAreasToElaTheme(QWidget *page)
{
    const QList<QAbstractScrollArea *> areas =
        page->findChildren<QAbstractScrollArea *>();
    for (QAbstractScrollArea *area : areas)
    {
        applyScrollAreaTheme(area, eTheme->getThemeMode());
        QObject::connect(
            eTheme, &ElaTheme::themeModeChanged, area,
            [area](ElaThemeType::ThemeMode themeMode)
            { applyScrollAreaTheme(area, themeMode); });
    }
}

} // namespace

MainWindow::MainWindow(Dialog *dialog, Tachie *tachie, QWidget *parent)
    : ElaWindow(parent)
{
    /*初始化窗口*/
    setWindowTitle("ZcChat2");
    setWindowIcon(QIcon(":/res/img/logo/logo.png"));
    setUserInfoCardVisible(false);

    /*创建窗口*/
    SettingChild_General *settingchild_generalWin = new SettingChild_General(this);
    settingchild_generalWin->show();
    addPageNode("通用设置", settingchild_generalWin, ElaIconType::Gear);
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

    //绑定各设置子页的滚动区域主题
    bindScrollAreasToElaTheme(settingchild_generalWin);
    bindScrollAreasToElaTheme(settingchild_llmWin);
    bindScrollAreasToElaTheme(settingchild_speechWin);
    bindScrollAreasToElaTheme(settingchild_vitsWin);
    bindScrollAreasToElaTheme(settingchild_pluginWin);
    bindScrollAreasToElaTheme(settingchild_charWin);
    bindScrollAreasToElaTheme(settingchild_aboutWin);

    //连接
    connect(settingchild_charWin, &SettingChild_Char::requestReloadCharSelect,
            tachie, &Tachie::SetTachieImg); //设置立绘图像（重载角色）
    connect(settingchild_charWin, &SettingChild_Char::requestSetTachieSize,
            tachie, &Tachie::SetTachieSize); //设置立绘大小
    connect(settingchild_charWin, &SettingChild_Char::requestResetTachieLoc,
            tachie, &Tachie::ResetTachieLoc); //重置立绘位置
    connect(settingchild_charWin, &SettingChild_Char::requestReloadAIConfig,
            dialog, &Dialog::ReloadAIConfig); //重载AI配置
    connect(settingchild_charWin, &SettingChild_Char::requestReloadSpeechConfig,
            dialog, &Dialog::ReloadSpeechInputConfig); //重载角色语音关键词
    //获取模型列表后刷新角色页的模型下拉框
    connect(settingchild_llmWin, &SettingChild_LLM::modelListRefreshed,
            settingchild_charWin, &SettingChild_Char::RefreshModelList); //刷新LLM模型列表
    connect(settingchild_vitsWin, &SettingChild_Vits::vitsModelListRefreshed,
            settingchild_charWin, &SettingChild_Char::RefreshVitsModelList); //刷新Vits模型列表
    connect(settingchild_speechWin, &SettingChild_Speech::speechConfigChanged,
            dialog, &Dialog::ReloadSpeechInputConfig); //刷新语音输入配置
    connect(settingchild_generalWin, &SettingChild_General::generalConfigChanged,
            dialog, &Dialog::ReloadGeneralConfig); //刷新通用配置
}

MainWindow::~MainWindow()
{
}
