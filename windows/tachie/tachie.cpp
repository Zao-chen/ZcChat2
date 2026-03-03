#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

#include "../../utils/DragHelper.h"
#include "ZcJsonLib.h"
#include <QTimer>
#include <QMouseEvent>
#include <QImage>
#include <QColor>
#include <QBitmap>
#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h> // 必须包含这个处理图像转换
#include <X11/extensions/shape.h>
#endif

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

    _scaledImg = scaledPixmap.toImage();
    
    //    实现点击穿透
    this->resize(scaledPixmap.size());
    ui->label_tachie1->setGeometry(0, 0, scaledPixmap.width(), scaledPixmap.height());
    
    #ifdef Q_OS_LINUX //这里是gemini3写的，我觉得在linux下的效果还不错，你可以到windows端测试一下
        // 1. 建立临时 X 链接
        Display *display = XOpenDisplay(nullptr);
        if (display) {
            Window window_id = static_cast<Window>(this->winId());

            // 2. 让 Qt 帮我们计算立绘的非透明区域矩形集合
            // createAlphaMask 会提取所有 Alpha > 0 的像素
            QRegion region(QBitmap::fromImage(scaledPixmap.toImage().createAlphaMask()));

            // 3. 将 Qt 的矩形集合转换为 X11 的矩形格式
            auto rects = region.begin(); // 获取矩形数组迭代器
            int count = region.rectCount();
            XRectangle *xrects = new XRectangle[count];
            
            for (int i = 0; i < count; ++i) {
                xrects[i].x = static_cast<short>(rects[i].x());
                xrects[i].y = static_cast<short>(rects[i].y());
                xrects[i].width = static_cast<unsigned short>(rects[i].width());
                xrects[i].height = static_cast<unsigned short>(rects[i].height());
            }

            // 4. 【核心】告诉 X Server 仅在这些矩形内拦截鼠标
            // ShapeInput 模式保证了视觉不被裁剪（无锯齿），只有判定被裁剪
            XShapeCombineRectangles(display, window_id, ShapeInput, 0, 0, xrects, count, ShapeSet, YXBanded);

            // 5. 释放资源
            delete[] xrects;
            XCloseDisplay(display);
        }
    #else
        // Windows 下维持原样
        this->setMask(scaledPixmap.mask());
    #endif
}


// 2. 添加 mousePressEvent
void Tachie::mousePressEvent(QMouseEvent *event)
{
    if (!_scaledImg.isNull() && 
        event->pos().x() >= 0 && event->pos().x() < _scaledImg.width() &&
        event->pos().y() >= 0 && event->pos().y() < _scaledImg.height()) 
    {
        // 使用pixelColor兼容性更好
        int alpha = _scaledImg.pixelColor(event->pos()).alpha();

        if (alpha < 10) {
            event->ignore(); 
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

//重置立绘位置
void Tachie::ResetTachieLoc()
{
    this->move(0, 0);
}
