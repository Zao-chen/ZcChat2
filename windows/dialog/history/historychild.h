#ifndef HISTORYCHILD_H
#define HISTORYCHILD_H

#include <QWidget>

namespace Ui
{
class historychild;
}

class historychild : public QWidget
{
    Q_OBJECT

  public:
    explicit historychild(int historyIndex, const QString &name, const QString &msg,
                          QWidget *parent = nullptr);
    ~historychild();

  signals:
    void jumpRequested(int historyIndex);

  private slots:
    void on_pushButton_jump_clicked();

  private:
    Ui::historychild *ui;
    int m_historyIndex = -1;
};

#endif //HISTORYCHILD_H
