#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

Tachie::Tachie(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Tachie)
{
    ui->setupUi(this);

    /*无边框设置*/
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    QPixmap pixmap;
    pixmap.load(CharacterTachiePath + "/default.png");
    //缩放新图片并设置到 label
    QPixmap scaledPixmap = pixmap.scaled(
        pixmap.width() * 1,
        pixmap.height() * 1,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    ui->label_tachie1->setPixmap(scaledPixmap);
}

Tachie::~Tachie()
{
    delete ui;
}
