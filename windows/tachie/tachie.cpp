#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

#include "../../utils/DragHelper.h"
#include "zcjsonlib.h"
#include <QTimer>

Tachie::Tachie(QWidget *parent)
    : QWidget(parent), ui(new Ui::Tachie)
{
    /*窗口设置*/
    ui->setupUi(this);
    //无边框
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //窗口拖拽
    new DragHelper(this);

    //延迟加载立绘
    QTimer::singleShot(0, this, [this]()
                       { SetTachieImg("default"); });
}

Tachie::~Tachie()
{
    delete ui;
}

//设置立绘
void Tachie::SetTachieImg(QString TachieName)
{
    NowTachie.load(ReadCharacterTachiePath() + "/" + TachieName + ".png");
    //读取立绘大小
    ZcJsonLib charUserConfig(ReadCharacterUserConfigPath());
    SetTachieSize(charUserConfig.value("tachieSize").toString().toInt());
}
//设置窗口大小并重载立绘
void Tachie::SetTachieSize(int size)
{
    qInfo() << "设置立绘大小为" << size;
    //缩放新图片并设置到 label
    QPixmap scaledPixmap = NowTachie.scaled(
        NowTachie.size() * (size / 100.0),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);
    ui->label_tachie1->setPixmap(scaledPixmap);
}

//重置立绘位置
void Tachie::ResetTachieLoc()
{
    this->move(0, 0);
}
