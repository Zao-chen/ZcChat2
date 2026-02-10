#include "dialog.h"
#include "ui_dialog.h"

#include "../../utils/CustomScrollBinder.h"
#include "../../GlobalConstants.h"

#include <QSettings>
#include <QPainterPath>
#include <QPainter>
#include <QMouseEvent>

/*窗口的绘制*/
void Dialog::paintEvent(QPaintEvent *event)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    QRectF rect(5, 5, this->width() - 10, this->height() - 10);
    path.addRoundedRect(rect, 15, 15);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillPath(path, QBrush(Qt::white));
    QColor color(0, 0, 0, 50);
    for (int i = 0; i < 5; i++)
    {
        QPainterPath shadowPath;
        shadowPath.setFillRule(Qt::WindingFill); //使用圆角矩形而不是普通矩形绘制阴影
        QRectF shadowRect((5 - i), (5 - i) , this->width() - (5 - i) * 2, this->height() - (5 - i) * 2);
        shadowPath.addRoundedRect(shadowRect, 15, 15); //添加圆角矩形路径
        color.setAlpha(50 - qSqrt(i) * 22); //增加透明度效果，模拟阴影逐渐变淡
        painter.setPen(color);
        painter.drawPath(shadowPath); //绘制阴影路径
    }
}
/*窗口的移动*/
void Dialog::mousePressEvent(QMouseEvent *event) //鼠标按下事件
{

    isLeftPressDown = true;
    this->mouseGrabber(); //返回当前抓取鼠标输入的窗口
    m_movePoint = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
}
void Dialog::mouseMoveEvent(QMouseEvent *event) //鼠标移动事件
{
    if(isLeftPressDown) //按下左键时移动到鼠标
    {
        move(event->globalPosition().toPoint() - m_movePoint);
        event->accept();
    }
}
void Dialog::mouseReleaseEvent(QMouseEvent *event) //鼠标释放事件
{
    if (event->button() == Qt::LeftButton)
    {
        isLeftPressDown = false;
        this->releaseMouse(); //释放鼠标抓取
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}
/*初始化窗口*/
void Dialog::initWindow()
{
    /*窗口初始化*/
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowOpacity(0.95);
    setAttribute(Qt::WA_TranslucentBackground);
    /*内容初始化*/
    ui->pushButton_next->hide();
    ui->verticalScrollBar->hide();
    //TextEdit的滚动条
    new CustomScrollBinder(ui->textEdit, ui->verticalScrollBar, 5, this);
}

/*构建窗口*/
Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    initWindow();


    //配置文件
    QSettings settings(Settingpath, QSettings::IniFormat);

    /*初始化AI*/
    ai = new AiProvider(this);
    ai->setServiceType(AiProvider::DeepSeek);
    ai->setApiKey(settings.value("llm/deepseek/key").toString());  // 替换成你的 Key
    //接收回复
    connect(ai, &AiProvider::replyReceived, [=](const QString &reply)
    {
        ui->pushButton_next->show();
        ui->textEdit->setText(reply);
    });
    //错误处理
    connect(ai, &AiProvider::errorOccurred, [=](const QString &error)
    {
        qDebug()<<error;
    });
}

/*解构窗口*/
Dialog::~Dialog()
{
    delete ui;
}

/*按键相关*/
void Dialog::keyPressEvent(QKeyEvent* event)
{
    keys.append(event->key());
}
void Dialog::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return)
        if (!keys.contains(Qt::Key_Shift)) //过滤Shift换行
        {
            //对话框设置
            ui->label_name->setText("她");
            ui->textEdit->setEnabled(false);
            ui->pushButton_next->hide();
            //去除换行
            QTextCursor cursor=ui->textEdit->textCursor(); //得到当前text的光标
            if(cursor.hasSelection()) cursor.clearSelection();
            cursor.deletePreviousChar(); //删除前一个字符
            ai->chat(ui->textEdit->toPlainText());
            ui->textEdit->setText("……");
        }
    keys.removeAll(event->key());
}

void Dialog::on_pushButton_next_clicked()
{
    ui->label_name->setText("你");
    ui->textEdit->clear();
    ui->textEdit->setEnabled(true);
    ui->pushButton_next->hide();
}

