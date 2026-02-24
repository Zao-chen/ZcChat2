#include "dialog.h"
#include "ui_dialog.h"

#include "../../GlobalConstants.h"

#include "../../utils/CustomScrollBinder.h"
#include "../../utils/DragHelper.h"

#include "ZcJsonLib.h"
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
    new CustomScrollBinder(ui->textEdit, ui->verticalScrollBar, 5, this); //TextEdit的滚动条
    new DragHelper(this); // 给窗口添加拖拽功能
}

/*构建窗口*/
Dialog::Dialog(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    initWindow();
    ai = new AiProvider(this);
    ReloadAIConfig();

    ZcJsonLib config(JsonSettingPath);

    QString apiKey = config.value("llm/DeepSeek/ApiKey").toString();  // 替换成你的 Key
    qDebug()<<apiKey;
    ai->setApiKey(apiKey);
    //接收回复
    connect(ai, &AiProvider::replyReceived, [=](const QString &reply)
            {
                ui->pushButton_next->show();
                ui->textEdit->setText(reply.section('|', 1, 1)); //提取中文内容并显示
                emit requestSetCharTachie(reply.section('|', 0, 0)); //提取心情并发出信号
            });
    //错误处理
    connect(ai, &AiProvider::errorOccurred, [=](const QString &error)
            {
                ui->pushButton_next->show();
                ui->textEdit->setText(error);
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
            //读取心情列表
            QDir dir(ReadCharacterTachiePath());
            QStringList nameFilters;
            nameFilters << "*.png" << "*.jpg" << "*.jpeg";
            QStringList fileNames = dir.entryList(nameFilters, QDir::Files);
            QString nameListStr;
            for (const QString &fileName : fileNames)
            {
                nameListStr += fileName.section('.', 0, 0) + ", ";
            }
            //设置系统提示词
            ai->setSystemPrompt(
                QStringLiteral("你是一个桌宠 AI，输出内容必须严格按照以下格式：\n"
                               "心情|中文|日语\n\n"
                               "要求：\n"
                               "1. 心情必须从以下列表中选择：")+ nameListStr + "\n"
              + QStringLiteral("2. 中文是桌宠此刻想表达的内容\n"
                               "3. 日语是中文内容的对应翻译\n"
                               "4. 输出中不能有多余内容或解释，严格用“|”分隔\n\n"
                               "示例输出：\n"
                               "快乐|今天的天气真好呀！|今日はいい天気ですね！\n"
                               "生气|为什么一直打扰我！|なんでずっと邪魔するの！"
                               )
            );
            ai->chat(ui->textEdit->toPlainText());
            ui->textEdit->setText("……");
        }
    keys.removeAll(event->key());
}

/*点击继续*/
void Dialog::on_pushButton_next_clicked()
{
    ui->label_name->setText("你");
    ui->textEdit->clear();
    ui->textEdit->setEnabled(true);
    ui->pushButton_next->hide();
}

/*开关窗口*/
void Dialog::ToggleVisible()
{
    setVisible(!isVisible());
}

/*重载ai配置*/
void Dialog::ReloadAIConfig()
{
    /*AI初始化*/
    ZcJsonLib CharConfig(ReadCharacterUserConfigPath());
    //读取当前角色的服务商选择
    QString serverSelect = CharConfig.value("serverSelect").toString();
    if(serverSelect == "DeepSeek")
        ai->setServiceType(AiProvider::DeepSeek);
    else if(serverSelect == "OpenAI")
        ai->setServiceType(AiProvider::OpenAI);
    else
        ai->setServiceType(AiProvider::DeepSeek);
    //读取当前角色的模型选择
    QString modelSelect = CharConfig.value("modelSelect").toString();
    ai->setModel(modelSelect);
}
