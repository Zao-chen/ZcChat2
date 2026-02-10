#include "windows/dialog/dialog.h"
#include "windows/tachie/tachie.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    /*窗口创建*/
    Dialog dialogWin;
    dialogWin.show();
    Tachie tachieWin;
    tachieWin.show();

    /*一些绑定*/
    QObject::connect(&tachieWin, &Tachie::toggleVisible,
                     &dialogWin, &Dialog::toggleVisible);

    return a.exec();
}
