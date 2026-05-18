#include "tachie.h"
#include "ui_tachie.h"

#include "../../GlobalConstants.h"

#include "../../utils/DragHelper.h"
#include "ZcJsonLib.h"
#include <QAbstractAnimation>
#include <QBitmap>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QEasingCurve>
#include <QFileInfo>
#include <QImage>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QVariantAnimation>
#include <memory>

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h> //必须包含这个处理图像转换
#include <X11/extensions/shape.h>
#endif

Tachie::Tachie(QWidget *parent)
    : QWidget(parent), ui(new Ui::Tachie)
{
    /*窗口设置*/
    ui->setupUi(this);
    // 立绘 label 改为绝对定位，避免布局系统与动画 setGeometry 冲突。
    if (ui->gridLayout)
        ui->gridLayout->removeWidget(ui->label_tachie1);
    ui->label_tachie1->setParent(this);

    //无边框
    setAttribute(Qt::WA_TranslucentBackground);
    Qt::WindowFlags flags = Qt::Tool | Qt::FramelessWindowHint |
                            Qt::WindowStaysOnTopHint;
#ifdef Q_OS_LINUX
    //避免窗口管理器限制拖拽范围（如屏幕边缘约束）。
    flags |= Qt::X11BypassWindowManagerHint;
#endif
    setWindowFlags(flags);
    //窗口拖拽
    new DragHelper(this);

    //延迟加载立绘
    QTimer::singleShot(0, this, [this]()
                       { SetTachieImg("default"); });

    //初始化动画插件索引
    m_animePluginManager.Reload();
}

Tachie::~Tachie()
{
    if (m_activeAnimationGroup)
    {
        m_activeAnimationGroup->stop();
        delete m_activeAnimationGroup;
        m_activeAnimationGroup = nullptr;
    }
    delete ui;
}

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
void Tachie::ApplyInteractiveRegion(const QRegion &region)
{
#ifdef Q_OS_LINUX
    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return;

    Window window_id = static_cast<Window>(this->winId());

    const int count = region.rectCount();
    if (count <= 0)
    {
        XShapeCombineRectangles(display, window_id, ShapeInput, 0, 0, nullptr, 0,
                                ShapeSet, YXBanded);
        XCloseDisplay(display);
        return;
    }

    auto rects = region.begin();
    QVector<XRectangle> xrects;
    xrects.resize(count);
    for (int i = 0; i < count; ++i)
    {
        const QRect &rect = rects[i];
        xrects[i].x = static_cast<short>(rect.x());
        xrects[i].y = static_cast<short>(rect.y());
        xrects[i].width = static_cast<unsigned short>(rect.width());
        xrects[i].height = static_cast<unsigned short>(rect.height());
    }

    XShapeCombineRectangles(display, window_id, ShapeInput, 0, 0, xrects.data(),
                            count, ShapeSet, YXBanded);
    XCloseDisplay(display);
#else
    setMask(region);
#endif
}

void Tachie::ApplyInteractiveRegionFromImage()
{
    if (_scaledImg.isNull())
        return;

    QRegion region(QBitmap::fromImage(_scaledImg.createAlphaMask()));
    region.translate(_scaledImgTopLeft);
    ApplyInteractiveRegion(region);
}

void Tachie::ApplyInteractiveRegionFullWindow()
{
    ApplyInteractiveRegion(QRegion(QRect(0, 0, width(), height())));
}
#endif

//设置立绘
void Tachie::SetTachieImg(QString TachieName)
{
    const QString tachieDirPath = ReadCharacterTachiePath();
    if (tachieDirPath.isEmpty())
    {
        return;
    }

    const QString normalizedName = TachieName.trimmed();
    QPixmap loadedPixmap;
    bool loaded = false;

    //兼容 AI 返回无扩展名和带扩展名两种情况，同时支持 png/jpg/jpeg。
    QFileInfo inputInfo(normalizedName);
    QStringList candidates;
    if (inputInfo.suffix().isEmpty())
    {
        candidates << (normalizedName + ".png") << (normalizedName + ".jpg")
                   << (normalizedName + ".jpeg");
    }
    else
    {
        candidates << normalizedName;
    }

    QDir tachieDir(tachieDirPath);
    for (const QString &candidate : candidates)
    {
        const QString filePath = tachieDir.filePath(candidate);
        if (QFileInfo::exists(filePath) && loadedPixmap.load(filePath))
        {
            loaded = true;
            break;
        }
    }

    //再做一次按文件名（不区分大小写）的兜底匹配，避免 AI 返回大小写不一致。
    if (!loaded)
    {
        const QStringList files = tachieDir.entryList(
            QStringList() << "*.png" << "*.jpg" << "*.jpeg", QDir::Files);
        for (const QString &fileName : files)
        {
            if (QFileInfo(fileName).completeBaseName().compare(
                    normalizedName, Qt::CaseInsensitive) != 0)
            {
                continue;
            }
            if (loadedPixmap.load(tachieDir.filePath(fileName)))
            {
                loaded = true;
                break;
            }
        }
    }

    if (loaded)
    {
        NowTachie = loadedPixmap;
    }
    else if (NowTachie.isNull())
    {
        qWarning() << "立绘加载失败:" << normalizedName;
        return;
    }

    //读取立绘大小
    ZcJsonLib charUserConfig(ReadCharacterUserConfigPath());
    SetTachieSize(charUserConfig.value("tachieSize").toString().toInt());
    RestoreTachieLoc();

    //按当前动作尝试播放绑定动画
    QString actionName = QFileInfo(normalizedName).completeBaseName();
    if (!actionName.isEmpty())
        TryPlayAnimationForAction(actionName);
}

/*播放动画*/
void Tachie::TryPlayAnimationForAction(const QString &actionName)
{
    const QString charName = ReadNowSelectChar();
    if (charName.isEmpty() || charName == "未选择")
        return;

    //动作绑定存放于角色资源配置
    ZcJsonLib charAssetConfig(CharacterAssestPath + "/" + charName +
                              "/config.json");
    const QJsonObject animationMap =
        charAssetConfig.value("tachieAnimations", QJsonObject()).toObject();
    const QString uniqueKey = animationMap.value(actionName).toString().trimmed();
    if (uniqueKey.isEmpty())
        return;

    //每次播放前刷新索引，确保刚导入/删除插件也生效
    m_animePluginManager.Reload();

    AnimePluginDefinition plugin;
    AnimePluginAnimation animation;
    if (!m_animePluginManager.TryGetAnimationByUniqueKey(uniqueKey, plugin,
                                                         animation))
    {
        qWarning() << "动画绑定无效:" << actionName << uniqueKey;
        return;
    }

    //停止上一个动画，避免并发导致位置/透明度冲突
    if (m_activeAnimationGroup)
    {
        m_activeAnimationGroup->stop();
        m_activeAnimationGroup->deleteLater();
        m_activeAnimationGroup = nullptr;
    }

    QSequentialAnimationGroup *seq = new QSequentialAnimationGroup(this);

    struct ScaleSequenceState
    {
        QRect baseImageRect;
        bool initialized = false;
    };
    auto scaleSequenceState = std::make_shared<ScaleSequenceState>();

    auto ensureScaleBaseInitialized = [this, scaleSequenceState]()
    {
        if (scaleSequenceState->initialized)
            return;
        // 锁定整段缩放动画的基准矩形，避免多 step 累积漂移。
        scaleSequenceState->baseImageRect = ui->label_tachie1->geometry();
        scaleSequenceState->initialized = true;
    };

    for (const AnimePluginStep &step : animation.steps)
    {
        const int durationMs = qMax(1, static_cast<int>(step.durationSec * 1000.0));

        if (step.type == AnimePluginStep::Type::Move)
        {
            struct MoveState
            {
                QPoint basePos;
                bool initialized = false;
            };
            auto moveState = std::make_shared<MoveState>();

            QVariantAnimation *moveAnim = new QVariantAnimation(seq);
            moveAnim->setDuration(durationMs);
            moveAnim->setEasingCurve(QEasingCurve::Linear);
            moveAnim->setStartValue(0.0);
            moveAnim->setEndValue(1.0);
            connect(moveAnim, &QVariantAnimation::valueChanged, this,
                    [this, moveState, step](const QVariant &v)
                    {
                        if (!moveState->initialized)
                        {
                            moveState->basePos = this->pos();
                            moveState->initialized = true;
                        }

                        const double progress = v.toDouble();
                        const int dx = qRound(step.x * progress);
                        const int dy = qRound(step.y * progress);
                        this->move(moveState->basePos + QPoint(dx, dy));
                    });
            seq->addAnimation(moveAnim);
            continue;
        }

        if (step.type == AnimePluginStep::Type::Opacity)
        {
            QPropertyAnimation *opacityAnim =
                new QPropertyAnimation(this, "windowOpacity", seq);
            opacityAnim->setDuration(durationMs);
            opacityAnim->setEasingCurve(QEasingCurve::Linear);
            opacityAnim->setStartValue(step.from);
            opacityAnim->setEndValue(step.to);
            seq->addAnimation(opacityAnim);
            continue;
        }

        if (step.type == AnimePluginStep::Type::Scale)
        {
            QVariantAnimation *scaleAnim = new QVariantAnimation(seq);
            scaleAnim->setDuration(durationMs);
            scaleAnim->setEasingCurve(QEasingCurve::Linear);
            scaleAnim->setStartValue(step.scaleFrom);
            scaleAnim->setEndValue(step.scaleTo);

            auto applyScaleFrame = [this, scaleSequenceState](double factor)
            {
                // 保护倍率，避免异常值导致图片瞬间过大。
                const double safeFactor = qBound(0.05, factor, 2.0);
                const int w = qMax(
                    1, qRound(scaleSequenceState->baseImageRect.width() * safeFactor));
                const int h = qMax(
                    1, qRound(scaleSequenceState->baseImageRect.height() * safeFactor));

                // 固定窗口，仅在画布中心缩放，保证始终从中心放大/缩小。
                const QPointF center(width() / 2.0, height() / 2.0);
                const int x = qRound(center.x() - w / 2.0);
                const int y = qRound(center.y() - h / 2.0);

                if (!NowTachie.isNull())
                {
                    const QPixmap scaledPixmap =
                        NowTachie.scaled(w, h, Qt::IgnoreAspectRatio,
                                         Qt::SmoothTransformation);
                    ui->label_tachie1->setPixmap(scaledPixmap);
                    ui->label_tachie1->setGeometry(x, y, w, h);
                    _scaledImg = scaledPixmap.toImage();
                    _scaledImgTopLeft = QPoint(x, y);
                }
            };

            connect(scaleAnim, &QVariantAnimation::valueChanged, this,
                    [ensureScaleBaseInitialized, applyScaleFrame](const QVariant &v)
                    {
                        ensureScaleBaseInitialized();

                        applyScaleFrame(v.toDouble());
                    });

            connect(scaleAnim, &QVariantAnimation::finished, this,
                    [ensureScaleBaseInitialized, applyScaleFrame, step]()
                    {
                        ensureScaleBaseInitialized();
                        // 每一步结束时吸附到目标值，消除帧步进带来的残余误差。
                        applyScaleFrame(step.scaleTo);
                    });
            seq->addAnimation(scaleAnim);
        }
    }

    if (seq->animationCount() <= 0)
    {
        seq->deleteLater();
        return;
    }

    m_activeAnimationGroup = seq;
    connect(seq, &QSequentialAnimationGroup::finished, this,
            [this]()
            { m_activeAnimationGroup = nullptr; });
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

//设置窗口大小并重载立绘
void Tachie::SetTachieSize(int size)
{
    constexpr double kCanvasScale = 2.0;
    const int safeSize = (size <= 0) ? 100 : size;
    qInfo() << "设置立绘大小为" << safeSize;

    if (NowTachie.isNull())
    {
        return;
    }

    //缩放新图片并设置到 label
    QPixmap scaledPixmap =
        NowTachie.scaled(NowTachie.size() * (safeSize / 100.0),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 预留 200% 画布，缩放动画只动图片层，不改窗口几何，避免抖动。
    const int canvasW = qMax(1, qRound(scaledPixmap.width() * kCanvasScale));
    const int canvasH = qMax(1, qRound(scaledPixmap.height() * kCanvasScale));
    const int imgX = (canvasW - scaledPixmap.width()) / 2;
    const int imgY = (canvasH - scaledPixmap.height()) / 2;

    this->resize(canvasW, canvasH);
    ui->label_tachie1->setPixmap(scaledPixmap);
    ui->label_tachie1->setGeometry(imgX, imgY, scaledPixmap.width(),
                                   scaledPixmap.height());

    _scaledImg = scaledPixmap.toImage();
    _scaledImgTopLeft = QPoint(imgX, imgY);

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) //这里保留透明输入区域逻辑，macOS 也沿用同一套 region 处理
    ApplyInteractiveRegionFromImage();
#else
    //Windows 下不裁剪窗口形状，避免半透明边缘被硬裁切后出现“略微缩小/边缘异常”。
    this->clearMask();
#endif
}

//鼠标按下
void Tachie::mousePressEvent(QMouseEvent *event)
{
    const QPoint pos = event->pos();
    const QPoint imgPos = pos - _scaledImgTopLeft;
    const QRect imageBounds(QPoint(0, 0), _scaledImg.size());
    if (_scaledImg.isNull() || !imageBounds.contains(imgPos))
    {
        event->ignore();
        return;
    }

    const int alpha = _scaledImg.pixelColor(imgPos).alpha();
    if (alpha < 10)
    {
        event->ignore();
        return;
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    //拖动时扩大输入区域，避免鼠标离开形状区域后丢失拖拽。
    ApplyInteractiveRegionFullWindow();
#endif

    QWidget::mousePressEvent(event);
}

//鼠标抬起
void Tachie::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    ApplyInteractiveRegionFromImage();
#endif

    //仅在初始化恢复完成后，且左键释放时保存一次位置。
    if (!_tachiePosRestoreDone || event->button() != Qt::LeftButton)
        return;

    SaveTachieLoc(); //保存立绘位置
}

//重置立绘位置
void Tachie::ResetTachieLoc()
{
    this->move(0, 0);
    SaveTachieLoc(); //保存立绘位置
}

//保存立绘位置
void Tachie::SaveTachieLoc()
{
    const QString charName = ReadNowSelectChar();
    if (charName.isEmpty() || charName == "未选择")
        return;

    QSettings settings(IniSettingPath, QSettings::IniFormat);
    settings.setValue(QString("tachie/%1/posX").arg(charName), this->x());
    settings.setValue(QString("tachie/%1/posY").arg(charName), this->y());
}
//读取设置立绘位置
void Tachie::RestoreTachieLoc()
{
    const QString charName = ReadNowSelectChar();
    if (charName.isEmpty() || charName == "未选择")
    {
        _tachiePosRestoreDone = false;
        return;
    }

    QSettings settings(IniSettingPath, QSettings::IniFormat);
    const QString keyX = QString("tachie/%1/posX").arg(charName);
    const QString keyY = QString("tachie/%1/posY").arg(charName);

    if (!settings.contains(keyX) || !settings.contains(keyY))
    {
        //没有历史位置时标记恢复完成，后续用户拖动可直接保存。
        _tachiePosRestoreDone = true;
        return;
    }

    //恢复阶段不触发 mouseReleaseEvent 保存，直接移动即可。
    this->move(settings.value(keyX).toInt(), settings.value(keyY).toInt());
    _tachiePosRestoreDone = true;
}
