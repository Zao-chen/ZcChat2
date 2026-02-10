#include "windows/dialog/dialog.h"
#include "windows/tachie/tachie.h"
#include "windows/setting/mainwindow.h"

#include <QApplication>
#include <QMenu>
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
    QObject::connect(&tachieWin, &Tachie::toggleVisible, &dialogWin, &Dialog::toggleVisible);

    /*托盘*/
    QSystemTrayIcon tray;
    tray.setIcon(QIcon(":/res/img/logo/logo.png"));
    tray.setToolTip("ZcChat2");
    QMenu trayMenu;
    QAction *actionSettings = trayMenu.addAction("设置");
    QAction *actionQuit = trayMenu.addAction("退出");
    tray.setContextMenu(&trayMenu);
    tray.show();
    //设置界面懒加载
    QObject::connect(actionSettings, &QAction::triggered, [&]()
    {
        if (!settings) settings = new MainWindow(&dialogWin);
        settings->show();
        settings->raise();
        settings->activateWindow();
    });
    //退出程序
    QObject::connect(actionQuit, &QAction::triggered, &a, &QApplication::quit);

    return a.exec();
}
