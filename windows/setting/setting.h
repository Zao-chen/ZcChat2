#ifndef SETTING_H
#define SETTING_H

#include <QMainWindow>
#include "ElaWindow.h"

#include "../tachie/tachie.h"
#include "../dialog/dialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    MainWindow(Dialog *dialog, Tachie *tachie, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};
#endif // SETTING_H
