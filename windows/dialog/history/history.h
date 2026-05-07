#ifndef HISTORY_H
#define HISTORY_H

#include <QWidget>

namespace Ui
{
class history;
}

class history : public QWidget
{
    Q_OBJECT

  public:
    explicit history(QWidget *parent = nullptr);
    void addChildWindow(int historyIndex, const QString &name, const QString &msg);
    void clearHistory();
    ~history();

  signals:
    void jumpToHistory(int historyIndex);

  private:
    Ui::history *ui;
    virtual void paintEvent(QPaintEvent *event) override;
};

#endif //HISTORY_H
