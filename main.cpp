#include "windows/pet/dialog.h"
#include "windows/tachie/tachie.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Dialog w;
    w.show();

    Tachie t;
    t.show();

    return a.exec();
}
