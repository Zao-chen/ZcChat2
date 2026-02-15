#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

#include "../../utils/DragHelper.h"

Tachie::Tachie(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tachie)
{
    ui->setupUi(this);
    /*无边框设置*/
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    new DragHelper(this); //给窗口添加拖拽功能

    ReloadCharSelect(); //重载立绘
}

Tachie::~Tachie()
{
    delete ui;
}

void Tachie::ReloadCharSelect()
{
    qDebug()<<"重载立绘";
    QPixmap pixmap;
    pixmap.load(ReadCharacterTachiePath() + "/default.png");
    //缩放新图片并设置到 label
    QPixmap scaledPixmap = pixmap.scaled(
        pixmap.width() * 1,
        pixmap.height() * 1,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    ui->label_tachie1->setPixmap(scaledPixmap);
}
