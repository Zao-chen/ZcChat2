#include "dialog.h"
#include "history/history.h"
#include "ui_dialog.h"

#include "../../GlobalConstants.h"

#include "../../utils/CustomScrollBinder.h"
#include "../../utils/DragHelper.h"

#include "ZcJsonLib.h"
#include <QFile>
#include <QJsonArray>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>

#include <QAudioOutput>
#include <QGraphicsOpacityEffect>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QTemporaryFile>
#include <QWheelEvent>

/*窗口的绘制*/
void Dialog::paintEvent(QPaintEvent *event) {
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
    shadowPath.setFillRule(
        Qt::WindingFill); // 使用圆角矩形而不是普通矩形绘制阴影
    QRectF shadowRect((5 - i), (5 - i), this->width() - (5 - i) * 2,
                      this->height() - (5 - i) * 2);
    shadowPath.addRoundedRect(shadowRect, 15, 15); // 添加圆角矩形路径
    color.setAlpha(50 - qSqrt(i) * 22); // 增加透明度效果，模拟阴影逐渐变淡
    painter.setPen(color);
    painter.drawPath(shadowPath); // 绘制阴影路径
  }
}

/*初始化窗口*/
void Dialog::initWindow() {
  /*窗口初始化*/
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  setWindowOpacity(0.95);
  setAttribute(Qt::WA_TranslucentBackground);
  /*内容初始化*/
  ui->pushButton_next->hide();
  ui->verticalScrollBar->hide();
  new CustomScrollBinder(ui->textEdit, ui->verticalScrollBar, 5,
                         this); // TextEdit的滚动条
  new DragHelper(this);         // 给窗口添加拖拽功能
  ui->textEdit->installEventFilter(this);
  ui->textEdit->viewport()->installEventFilter(this);
}

/*打开关闭历史记录*/
void Dialog::handleWheelUp() {
  if (!isHistoryOpen)
    ui->pushButton_history->click();
}
void Dialog::handleWheelDown() {
  if (isHistoryOpen)
    ui->pushButton_history->click();
}

/*加载上下文历史*/
void Dialog::loadContextHistory() {
  m_contextHistory.clear();
  const QString contextPath = ReadCharacterContextPath();
  if (contextPath.isEmpty())
    return;

  ZcJsonLib contextConfig(contextPath);
  const QJsonArray historyArray =
      contextConfig.value("history", QJsonValue(QJsonArray())).toArray();
  for (const QJsonValue &value : historyArray) {
    const QString line = value.toString();
    if (!line.isEmpty())
      m_contextHistory.append(line);
  }
}

/*保存上下文历史*/
void Dialog::saveContextHistory() const {
  const QString contextPath = ReadCharacterContextPath();
  if (contextPath.isEmpty())
    return;

  const QFileInfo fileInfo(contextPath);
  QDir().mkpath(fileInfo.absolutePath());

  QJsonArray historyArray;
  for (const QString &line : m_contextHistory)
    historyArray.append(line);

  ZcJsonLib contextConfig(contextPath);
  contextConfig.setValue("history", QJsonValue(historyArray));
}

/*构建用户消息，包含上下文*/
QString Dialog::buildUserMessageWithContext(const QString &input) const {
  if (m_contextHistory.isEmpty())
    return input;

  return QStringLiteral(
             "以下是你和用户最近的对话，请延续上下文并保持人设一致：\n") +
         m_contextHistory.join("\n") + QStringLiteral("\n\n用户当前输入：") +
         input;
}

/*添加历史记录行*/
void Dialog::appendHistoryLine(const QString &line) {
  if (line.isEmpty())
    return;
  m_contextHistory.append(line);
}

/*构建窗口*/
Dialog::Dialog(QWidget *parent)
    : QWidget(parent), ui(new Ui::Dialog), historyWin(nullptr),
      isHistoryOpen(false) {
  ui->setupUi(this);
  initWindow();
  ai = new AiProvider(this);
  ReloadAIConfig();
  loadContextHistory();

  ZcJsonLib config(JsonSettingPath);

  QString apiKey =
      config.value("llm/DeepSeek/ApiKey").toString(); // 替换成你的 Key
  qDebug() << apiKey;
  ai->setApiKey(apiKey);
  lastPos = pos();
  // 接收回复
  connect(ai, &AiProvider::replyReceived, [=](const QString &reply) {
    const QString mood = reply.section('|', 0, 0);
    const QString chineseReply = reply.section('|', 1, 1);
    ui->pushButton_next->show();
    ui->textEdit->setText(chineseReply); // 提取中文内容并显示
    // 判断是否开启了Vits语音合成
    ZcJsonLib charConfig(ReadCharacterUserConfigPath());
    bool vitsEnable = charConfig.value("vitsEnable").toBool();
    if (vitsEnable)
      VitsGetAndPlay(chineseReply);  // 提取中文内容并进行语音合成播放
    emit requestSetCharTachie(mood); // 提取心情并发出信号

    if (!m_lastUserInput.isEmpty()) {
      appendHistoryLine(QStringLiteral("用户：") + m_lastUserInput);
      m_lastUserInput.clear();
    }
    appendHistoryLine(QStringLiteral("角色：") + chineseReply);
    saveContextHistory();
  });
  // 错误处理
  connect(ai, &AiProvider::errorOccurred, [=](const QString &error) {
    ui->pushButton_next->show();
    ui->textEdit->setText(error);
    m_lastUserInput.clear();
  });
}

/*解构窗口*/
Dialog::~Dialog() { delete ui; }

/*按键相关*/
void Dialog::keyPressEvent(QKeyEvent *event) { keys.append(event->key()); }
void Dialog::keyReleaseEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return)
    if (!keys.contains(Qt::Key_Shift)) // 过滤Shift换行
    {
      // 对话框设置
      ui->label_name->setText("她");
      ui->textEdit->setEnabled(false);
      ui->pushButton_next->hide();
      // 去除换行
      QTextCursor cursor = ui->textEdit->textCursor(); // 得到当前text的光标
      if (cursor.hasSelection())
        cursor.clearSelection();
      cursor.deletePreviousChar(); // 删除前一个字符
      const QString userInput = ui->textEdit->toPlainText().trimmed();
      if (userInput.isEmpty()) {
        ui->textEdit->clear();
        ui->textEdit->setEnabled(true);
        return;
      }
      // 读取心情列表
      QDir dir(ReadCharacterTachiePath());
      QStringList nameFilters;
      nameFilters << "*.png" << "*.jpg" << "*.jpeg";
      QStringList fileNames = dir.entryList(nameFilters, QDir::Files);
      QString nameListStr;
      for (const QString &fileName : fileNames) {
        nameListStr += fileName.section('.', 0, 0) + ", ";
      }
      ZcJsonLib roleConfig(CharacterAssestPath + "/" + ReadNowSelectChar() +
                           "/config.json");
      const QString characterPrompt =
          roleConfig.value("prompt").toString().trimmed();
      // 设置系统提示词
      QString systemPrompt;
      if (!characterPrompt.isEmpty())
        systemPrompt += QStringLiteral("角色设定：") + characterPrompt +
                        QStringLiteral("\n请始终保持该设定进行回复。\n\n");
      systemPrompt +=
          QStringLiteral("你是一个桌宠 AI，输出内容必须严格按照以下格式：\n"
                         "心情|中文|日语\n\n"
                         "要求：\n"
                         "1. 心情必须从以下列表中选择：") +
          nameListStr + "\n" +
          QStringLiteral("2. 中文是桌宠此刻想表达的内容\n"
                         "3. 日语是中文内容的对应翻译\n"
                         "4. 输出中不能有多余内容或解释，严格用“|”分隔\n\n"
                         "示例输出：\n"
                         "快乐|今天的天气真好呀！|今日はいい天気ですね！\n"
                         "生气|为什么一直打扰我！|なんでずっと邪魔するの！");
      ai->setSystemPrompt(systemPrompt);

      m_lastUserInput = userInput;
      ai->chat(buildUserMessageWithContext(userInput));
      ui->textEdit->setText("……");
    }
  keys.removeAll(event->key());
}

/*点击继续*/
void Dialog::on_pushButton_next_clicked() {
  ui->label_name->setText("你");
  ui->textEdit->clear();
  ui->textEdit->setEnabled(true);
  ui->pushButton_next->hide();
}

/*开关窗口*/
void Dialog::ToggleVisible() { setVisible(!isVisible()); }

/*重载ai配置*/
void Dialog::ReloadAIConfig() {
  /*AI初始化*/
  ZcJsonLib CharConfig(ReadCharacterUserConfigPath());
  // 读取当前角色的服务商选择
  QString serverSelect = CharConfig.value("serverSelect").toString();
  if (serverSelect == "DeepSeek")
    ai->setServiceType(AiProvider::DeepSeek);
  else if (serverSelect == "OpenAI")
    ai->setServiceType(AiProvider::OpenAI);
  else
    ai->setServiceType(AiProvider::DeepSeek);
  // 读取当前角色的模型选择
  QString modelSelect = CharConfig.value("modelSelect").toString();
  ai->setModel(modelSelect);

  loadContextHistory();
}

/*显示历史记录*/
void Dialog::on_pushButton_history_clicked() {
  if (!historyWin)
    historyWin = new history(this);

  historyWin->clearHistory();
  for (const QString &line : m_contextHistory) {
    if (line.startsWith(QStringLiteral("用户：")))
      historyWin->addChildWindow(QStringLiteral("你"), line.mid(3));
    else if (line.startsWith(QStringLiteral("角色：")))
      historyWin->addChildWindow(QStringLiteral("她"), line.mid(3));
    else
      historyWin->addChildWindow(QStringLiteral("记录"), line);
  }

  historyWin->move(this->x(), this->y() - historyWin->height());

  if (!isHistoryOpen) {
    historyWin->show();
    historyWin->raise();
    isHistoryOpen = true;

    QGraphicsOpacityEffect *opacityEffect =
        new QGraphicsOpacityEffect(historyWin);
    historyWin->setGraphicsEffect(opacityEffect);

    QRect startRect = historyWin->geometry();
    QRect endRect = startRect;
    startRect.moveTop(startRect.top() + 20);
    historyWin->setGeometry(startRect);
    opacityEffect->setOpacity(0.0);

    QPropertyAnimation *opacityAnim =
        new QPropertyAnimation(opacityEffect, "opacity");
    opacityAnim->setDuration(150);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);

    QPropertyAnimation *moveAnim =
        new QPropertyAnimation(historyWin, "geometry");
    moveAnim->setDuration(150);
    moveAnim->setStartValue(startRect);
    moveAnim->setEndValue(endRect);

    QParallelAnimationGroup *group = new QParallelAnimationGroup(historyWin);
    group->addAnimation(opacityAnim);
    group->addAnimation(moveAnim);
    group->start(QAbstractAnimation::DeleteWhenStopped);
  } else {
    isHistoryOpen = false;

    QRect startRect = historyWin->geometry();
    QRect endRect = startRect;
    endRect.moveTop(endRect.top() + 20);

    QGraphicsOpacityEffect *opacityEffect =
        qobject_cast<QGraphicsOpacityEffect *>(historyWin->graphicsEffect());
    if (!opacityEffect) {
      opacityEffect = new QGraphicsOpacityEffect(historyWin);
      historyWin->setGraphicsEffect(opacityEffect);
    }

    QPropertyAnimation *opacityAnim =
        new QPropertyAnimation(opacityEffect, "opacity");
    opacityAnim->setDuration(150);
    opacityAnim->setStartValue(1.0);
    opacityAnim->setEndValue(0.0);

    QPropertyAnimation *moveAnim =
        new QPropertyAnimation(historyWin, "geometry");
    moveAnim->setDuration(150);
    moveAnim->setStartValue(startRect);
    moveAnim->setEndValue(endRect);

    QParallelAnimationGroup *group = new QParallelAnimationGroup(historyWin);
    group->addAnimation(opacityAnim);
    group->addAnimation(moveAnim);
    connect(group, &QParallelAnimationGroup::finished, historyWin,
            &QWidget::hide);
    group->start(QAbstractAnimation::DeleteWhenStopped);
  }
}

void Dialog::moveEvent(QMoveEvent *event) {
  if (historyWin && historyWin->isVisible()) {
    QPoint offset = event->pos() - lastPos;
    historyWin->move(historyWin->pos() + offset);
  }
  lastPos = event->pos();
  QWidget::moveEvent(event);
}

void Dialog::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().y() > 0)
    handleWheelUp();
  else if (event->angleDelta().y() < 0)
    handleWheelDown();
  QWidget::wheelEvent(event);
}

bool Dialog::eventFilter(QObject *watched, QEvent *event) {
  if ((watched == ui->textEdit || watched == ui->textEdit->viewport()) &&
      event->type() == QEvent::Wheel) {
    QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
    if (wheelEvent->angleDelta().y() > 0)
      handleWheelUp();
    else if (wheelEvent->angleDelta().y() < 0)
      handleWheelDown();
    return true;
  }
  return QWidget::eventFilter(watched, event);
}

/*语音合成并播放*/
void Dialog::VitsGetAndPlay(QString text) {
  /*请求地址构建*/
  // 获取地址
  ZcJsonLib config(JsonSettingPath);
  QString apiUrl = config.value("vits/ApiUrl").toString();
  // 获取选中模型
  ZcJsonLib charConfig(ReadCharacterUserConfigPath());
  QString modelAndSpeaker = charConfig.value("vitsMasSelect").toString();
  QString model =
      modelAndSpeaker.section(" - ", 0, 0).trimmed().toLower(); // 提取模型名
  QString speaker =
      modelAndSpeaker.section(" - ", 2, 2).trimmed(); // 提取说话人
  // 构建请求 URL，对文本进行 URL 编码
  QString urlString = QString(apiUrl + "/voice/%2?id=%3&text=%1")
                          .arg(QString(QUrl::toPercentEncoding(text)))
                          .arg(QString(QUrl::toPercentEncoding(model)))
                          .arg(QString(QUrl::toPercentEncoding(speaker)));
  qInfo() << "语音合成请求" << modelAndSpeaker;
  // 创建网络管理器（建议作为类成员变量，避免重复创建）
  QNetworkAccessManager *manager = new QNetworkAccessManager();
  QNetworkRequest request(urlString);
  // 发送 GET 请求
  QNetworkReply *reply = manager->get(request);
  // 连接信号处理响应
  QObject::connect(reply, &QNetworkReply::finished, [=]() {
    if (reply->error() == QNetworkReply::NoError) {
      // 读取 MP3 数据
      QByteArray audioData = reply->readAll();
      // 创建临时文件保存音频
      QTemporaryFile *tempFile =
          new QTemporaryFile(QDir::tempPath() + "/vits_XXXXXX.mp3");
      if (tempFile->open()) {
        tempFile->write(audioData);
        tempFile->flush();
        // 创建播放器（建议作为类成员变量）
        QMediaPlayer *player = new QMediaPlayer();
        QAudioOutput *audioOutput = new QAudioOutput();
        player->setAudioOutput(audioOutput);
        // 设置音频源
        player->setSource(QUrl::fromLocalFile(tempFile->fileName()));
        // 播放
        player->play();
        // 播放完成后清理资源
        QObject::connect(player, &QMediaPlayer::playbackStateChanged,
                         [=](QMediaPlayer::PlaybackState state) {
                           if (state == QMediaPlayer::StoppedState) {
                             player->deleteLater();
                             tempFile->deleteLater();
                           }
                         });
        // 保持临时文件引用，防止过早删除
        QObject::connect(player, &QObject::destroyed,
                         [=]() { delete tempFile; });
      }
    }
    reply->deleteLater();
    manager->deleteLater();
  });
}
