#include "history.h"
#include "historychild.h"
#include "ui_history.h"

#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QVBoxLayout>

history::history(QWidget *parent) : QWidget(parent), ui(new Ui::history) {
  ui->setupUi(this);
  // 设置窗口属性
  // 无边框设置
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  setWindowOpacity(0.9);
  setAttribute(Qt::WA_TranslucentBackground);
  // 自动滚动到底部，便于查看最新记录
  ui->scrollArea->setWidgetResizable(true);
  connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this,
          [=]() {
            ui->scrollArea->verticalScrollBar()->setValue(
                ui->scrollArea->verticalScrollBar()->maximum());
          });
}

history::~history() { delete ui; }

/*清空历史*/
void history::clearHistory() {
  QVBoxLayout *layout =
      qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());
  if (!layout)
    return;

  while (QLayoutItem *item = layout->takeAt(0)) {
    if (item->widget())
      item->widget()->deleteLater();
    delete item;
  }
}

/*添加历史*/
void history::addChildWindow(const QString &name, const QString &msg) {
  QVBoxLayout *layout =
      qobject_cast<QVBoxLayout *>(ui->scrollAreaWidgetContents->layout());
  if (!layout)
    return;

  historychild *newChild = new historychild(name, msg, this);
  layout->addWidget(newChild);
}

/*圆角边框*/
void history::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event)
  QPainterPath path;
  path.setFillRule(Qt::WindingFill);
  QRectF rect(5, 5, this->width() - 10, this->height() - 10);
  path.addRoundedRect(rect, 15, 15);
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.fillPath(path, QBrush(Qt::white));
  QColor color(0, 0, 0, 50);
  for (int i = 0; i < 5; i++) {
    QPainterPath shadowPath;
    shadowPath.setFillRule(Qt::WindingFill);
    // 使用圆角矩形而不是普通矩形绘制阴影
    QRectF shadowRect((5 - i), (5 - i), this->width() - (5 - i) * 2,
                      this->height() - (5 - i) * 2);
    shadowPath.addRoundedRect(shadowRect, 15, 15);
    // 增加透明度效果，模拟阴影逐渐变淡
    color.setAlpha(50 - qSqrt(i) * 22);
    painter.setPen(color);
    painter.drawPath(shadowPath);
  }
}
