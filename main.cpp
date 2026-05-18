#include "windows/dialog/dialog.h"
#include "windows/setting/setting.h"
#include "windows/tachie/tachie.h"

#include "ElaApplication.h"
#include "ElaMenu.h"

#include "Version.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QStandardPaths>
#include <QSystemTrayIcon>

#include <QDir>

namespace
{
#ifdef Q_OS_MACOS
struct DefaultConfigEntry
{
    const char *resourcePath;
    const char *relativePath;
};

void CopyDefaultConfigIfMissing()
{
    const QString documentsRoot =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsRoot.isEmpty())
        return;

    const QString targetRoot = QDir(documentsRoot).filePath("ZcChat2");
    const QString targetIni = QDir(targetRoot).filePath("config.ini");
    if (QFile::exists(targetIni))
        return;

    const DefaultConfigEntry entries[] = {
        {":/default_config/ZcChat2/config.ini", "config.ini"},
        {":/default_config/ZcChat2/Plugin/Anime/Basic Animation Package.json",
         "Plugin/Anime/Basic Animation Package.json"},
        {":/default_config/ZcChat2/Character/Assets/test/config.json",
         "Character/Assets/test/config.json"},
        {":/default_config/ZcChat2/Character/Assets/test/Tachie/default.png",
         "Character/Assets/test/Tachie/default.png"},
        {":/default_config/ZcChat2/Character/UserConfig/test/config.json",
         "Character/UserConfig/test/config.json"},
    };

    for (const DefaultConfigEntry &entry : entries)
    {
        const QString outPath = QDir(targetRoot).filePath(entry.relativePath);
        const QFileInfo outInfo(outPath);
        if (!outInfo.dir().exists())
            QDir().mkpath(outInfo.dir().absolutePath());
        if (QFile::exists(outPath))
            continue;

        QFile inFile(QString::fromUtf8(entry.resourcePath));
        if (!inFile.open(QIODevice::ReadOnly))
            continue;

        QFile outFile(outPath);
        if (!outFile.open(QIODevice::WriteOnly))
            continue;

        outFile.write(inFile.readAll());
    }
}
#endif
} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif
    QApplication a(argc, argv);
#ifdef Q_OS_MACOS
    // Keep QLabel text readable when macOS switches to dark mode.
    QPalette labelPalette = a.palette();
    labelPalette.setColor(QPalette::WindowText, QColor(20, 20, 20));
    QApplication::setPalette(labelPalette, "QLabel");

    // Keep QTextEdit text/placeholder readable in dark mode.
    QPalette textEditPalette = a.palette();
    textEditPalette.setColor(QPalette::Text, QColor(20, 20, 20));
    textEditPalette.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    QApplication::setPalette(textEditPalette, "QTextEdit");

    CopyDefaultConfigIfMissing();
#endif
    a.setQuitOnLastWindowClosed(false);

    QCoreApplication::setApplicationName("ZcChat2");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("MyOrganization");
    qInfo() << "Debugging Output";

    /*窗口创建*/
    Dialog dialogWin;
    dialogWin.show();
    Tachie tachieWin;
    tachieWin.show();
    MainWindow *settings = nullptr;

    /*一些绑定*/
    //对话框的开启和关闭
    QObject::connect(&tachieWin, &Tachie::requestToggleVisible, &dialogWin,
                     &Dialog::ToggleVisible);
    //修改立绘图片
    QObject::connect(&dialogWin, &Dialog::requestSetCharTachie, &tachieWin,
                     &Tachie::SetTachieImg);

    /*托盘*/
    QSystemTrayIcon tray;
    tray.setIcon(QIcon(":/res/img/logo/logo.png"));
    tray.setToolTip("ZcChat2");
    ElaMenu trayMenu;
    QAction *actionSettings = trayMenu.addAction("设置");
    QAction *actionQuit = trayMenu.addAction("退出");
    tray.setContextMenu(&trayMenu);
    tray.show();
    //左键点击托盘打开设置
    QObject::connect(&tray, QOverload<QSystemTrayIcon::ActivationReason>::of(&QSystemTrayIcon::activated),
                     [&](QSystemTrayIcon::ActivationReason reason)
                     {
                         if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
                         {
                             if (!settings)
                             {
                                 eApp->init();
                                 settings = new MainWindow(&dialogWin, &tachieWin);
                             }
                             settings->show();
                             settings->raise();
                             settings->activateWindow();
                         }
                     });
    //设置界面懒加载
    QObject::connect(actionSettings, &QAction::triggered, [&]()
                     {
                         if (!settings)
                         {
                             eApp->init();
                             settings = new MainWindow(&dialogWin, &tachieWin);
                         }
                         settings->show();
                         settings->raise();
                         settings->activateWindow(); });
    //退出程序
    QObject::connect(actionQuit, &QAction::triggered, &a, &QApplication::quit);

    return a.exec();
}
