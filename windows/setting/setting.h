#ifndef SETTING_H
#define SETTING_H

#include "ElaWindow.h"

#include "../dialog/dialog.h"
#include "../tachie/tachie.h"

class MainWindow : public ElaWindow
{
    Q_OBJECT

  public:
    MainWindow(Dialog *dialog, Tachie *tachie, QWidget *parent = nullptr);
    ~MainWindow();
};
#endif //SETTING_H
