#ifndef TACHIE_H
#define TACHIE_H

#include <QWidget>

namespace Ui {
class Tachie;
}

class Tachie : public QWidget
{
    Q_OBJECT

public:
    explicit Tachie(QWidget *parent = nullptr);
    ~Tachie();

private:
    Ui::Tachie *ui;
};

#endif // TACHIE_H
