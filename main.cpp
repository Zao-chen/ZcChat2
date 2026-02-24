#include "windows/dialog/dialog.h"
#include "windows/tachie/tachie.h"
#include "windows/setting/setting.h"

#include "ElaMenu.h"
#include "ElaApplication.h"

#include <QApplication>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /*窗口创建*/
    Dialog dialogWin;
    dialogWin.show();
    Tachie tachieWin;
    tachieWin.show();
    MainWindow *settings = nullptr;

    /*一些绑定*/
    //对话框的开启和关闭
    QObject::connect(&tachieWin, &Tachie::requestToggleVisible, &dialogWin, &Dialog::ToggleVisible);
    //修改立绘图片
    QObject::connect(&dialogWin, &Dialog::requestSetCharTachie, &tachieWin, &Tachie::SetTachieImg);

    /*托盘*/
    QSystemTrayIcon tray;
    tray.setIcon(QIcon(":/res/img/logo/logo.png"));
    tray.setToolTip("ZcChat2");
    ElaMenu trayMenu;
    QAction *actionSettings = trayMenu.addAction("设置");
    QAction *actionQuit = trayMenu.addAction("退出");
    tray.setContextMenu(&trayMenu);
    tray.show();
    //设置界面懒加载
    QObject::connect(actionSettings, &QAction::triggered, [&]()
    {
        if (!settings)
        {
            eApp->init();
            settings = new MainWindow(&dialogWin, &tachieWin, &dialogWin);
        }
        settings->show();
        settings->raise();
        settings->activateWindow();
    });
    //退出程序
    QObject::connect(actionQuit, &QAction::triggered, &a, &QApplication::quit);

    return a.exec();
}
