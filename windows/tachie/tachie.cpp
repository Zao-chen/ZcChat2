#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

#include "../../utils/DragHelper.h"
#include <QTimer>

Tachie::Tachie(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tachie)
{
    /*窗口设置*/
    ui->setupUi(this);
    //无边框
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //窗口拖拽
    new DragHelper(this);

    //延迟加载立绘
    QTimer::singleShot(0, this, [this]() {
        SetCharTachie("default");
    });

}

Tachie::~Tachie()
{
    delete ui;
}

void Tachie::SetCharTachie(QString TachieName)
{
    qDebug()<<"重载立绘";
    QPixmap pixmap;
    pixmap.load(ReadCharacterTachiePath() + "/"+ TachieName +".png");
    //缩放新图片并设置到 label
    QPixmap scaledPixmap = pixmap.scaled(
        ui->label_tachie1->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    ui->label_tachie1->setPixmap(scaledPixmap);
}
