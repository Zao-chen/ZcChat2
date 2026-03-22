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

/*寻找句子分割点*/
static int findNextSentenceEnd(const QString &text, int start) {
  for (int i = qMax(0, start); i < text.size(); ++i) {
    const QChar ch = text.at(i);
    if (ch == QChar('.') || ch == QChar('!') || ch == QChar('?') ||
        ch == QChar('\n') || ch == QStringLiteral("。").at(0) ||
        ch == QStringLiteral("！").at(0) || ch == QStringLiteral("？").at(0) ||
        ch == QStringLiteral("、").at(0) || ch == QStringLiteral("；").at(0) ||
        ch == QChar(';'))
      return i;
  }
  return -1;
}

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
Dialog::Dialog(QWidget *parent) : QWidget(parent), ui(new Ui::Dialog) {
  ui->setupUi(this);
  initWindow();

  /*AI初始化*/
  ai = new AiProvider(this);
  ai->setStreamEnabled(true);

  /*Vits初始化*/
  m_vitsManager = new QNetworkAccessManager(this);
  m_vitsPlayer = new QMediaPlayer(this);
  m_vitsAudioOutput = new QAudioOutput(this);
  m_vitsPlayer->setAudioOutput(m_vitsAudioOutput);
  // 播放完成后播放下一条
  connect(m_vitsPlayer, &QMediaPlayer::playbackStateChanged, this,
          [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState) {
              if (m_vitsTempFile) {
                m_vitsTempFile->deleteLater();
                m_vitsTempFile = nullptr;
              }
              tryStartNextVitsPlayback();
            }
          });
  ReloadAIConfig();
  loadContextHistory();

  /*数据读取和初始化*/
  ZcJsonLib config(JsonSettingPath);
  QString apiKey = config.value("llm/DeepSeek/ApiKey").toString(); // 读取ApiKey
  ai->setApiKey(apiKey); // Todo: 根据不同服务类型设置不同的Key

  // 接收分块回复
  connect(ai, &AiProvider::replyChunkReceived, [=](const QString &chunk) {
    m_streamRawReply += chunk; // 追加

    /*提取中文*/
    const int firstSep = m_streamRawReply.indexOf('|'); // 寻找第一个分隔符
    if (firstSep < 0)
      return;
    const int secondSep =
        m_streamRawReply.indexOf('|',
                                 firstSep + 1); // 寻找第二个分隔符
    const int chineseEnd =
        secondSep < 0
            ? m_streamRawReply.size()
            : secondSep; // 如果没有找到第二个分隔符，就以当前字符串末尾为中文结束位置
    const QString chinesePartial = m_streamRawReply.mid(
        firstSep + 1, chineseEnd - firstSep - 1); // 提取中文部分
    // 更新中文的显示部分
    if (!chinesePartial.isEmpty() &&
        chinesePartial != m_streamDisplayedChinese) {
      m_streamDisplayedChinese = chinesePartial;
      ui->textEdit->setText(m_streamDisplayedChinese);
    }

    /*第二个分隔符处理*/
    if (m_streamVitsEnabled && m_streamVitsSentenceSplitEnabled &&
      secondSep >= 0) {
      const QString japanesePartial =
          m_streamRawReply.mid(secondSep + 1); // 提取日语的全部内容
      if (!japanesePartial.isEmpty()) {
        int sentenceEnd =
            findNextSentenceEnd(japanesePartial,
                                m_streamSynthCursor); // 初始化首个句尾位置
        while (sentenceEnd >= 0) {
          const QString sentence =
              japanesePartial
                  .mid(m_streamSynthCursor,
                       sentenceEnd - m_streamSynthCursor + 1)
                  .trimmed(); // 获取从上一次切分位置到当前句子结束位置的文本
          m_streamSynthCursor = sentenceEnd + 1; // 记录切分位置
          if (!sentence.isEmpty()) {
            VitsGetAndPlay(sentence); // 发送到语音合成
          }
          sentenceEnd = findNextSentenceEnd(
              japanesePartial,
              m_streamSynthCursor); // 继续查找下一句结束位置
        }
      }
    }
  });

  // 接收完整回复
  connect(ai, &AiProvider::replyReceived, [=](const QString &reply) {
    const QString finalReply = m_streamRawReply.isEmpty()
                                   ? reply
                                   : m_streamRawReply; // 确保使用完整结果
    // 解析回复
    const QString mood = finalReply.section('|', 0, 0).trimmed();
    const QString chineseReply = finalReply.section('|', 1, 1).trimmed();
    const QString japaneseReply = finalReply.section('|', 2, 2).trimmed();

    // 界面更新
    ui->pushButton_next->show();
    ui->textEdit->setText(chineseReply); // 提取中文内容并显示
    // 语音合成补漏或收尾生成
    if (m_streamVitsEnabled) {
      if (m_streamVitsSentenceSplitEnabled) {
        // 若最后一段不足一句（无句末标点），在结束回包时补一次合成。
        const QString remainJapanese =
            japaneseReply.mid(qMax(0, m_streamSynthCursor)).trimmed();
        if (!remainJapanese.isEmpty())
          VitsGetAndPlay(remainJapanese);
      } else {
        // 关闭切分后，仅在完整日语输出后一次性生成语音。
        if (!japaneseReply.isEmpty())
          VitsGetAndPlay(japaneseReply);
      }
    }
    emit requestSetCharTachie(mood); // 提取心情并发出信号

    // 历史记录写入
    if (!m_lastUserInput.isEmpty()) {
      appendHistoryLine(QStringLiteral("用户：") + m_lastUserInput);
      m_lastUserInput.clear();
    }
    appendHistoryLine(QStringLiteral("角色：") + chineseReply);
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

    // 重置内容
    m_streamRawReply.clear();
    m_streamDisplayedChinese.clear();
    m_streamVitsEnabled = false;
    m_streamSynthCursor = 0;
  });
  // 错误处理
  connect(ai, &AiProvider::errorOccurred, [=](const QString &error) {
    ui->pushButton_next->show();
    ui->textEdit->setText(error);
    m_lastUserInput.clear();
    m_streamRawReply.clear();
    m_streamDisplayedChinese.clear();
    m_streamVitsEnabled = false;
    m_streamSynthCursor = 0;
  });
}

/*解构窗口*/
Dialog::~Dialog() { delete ui; }

/*按键相关*/
void Dialog::keyPressEvent(QKeyEvent *event) { keys.append(event->key()); }
void Dialog::keyReleaseEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Return)
    /*发送对话请求*/
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

      /*一些东西的初始化*/
      m_lastUserInput = userInput;
        ZcJsonLib charConfig(ReadCharacterUserConfigPath());
        m_streamVitsEnabled = charConfig.value("vitsEnable").toBool();
        ZcJsonLib config(JsonSettingPath);
        m_streamVitsSentenceSplitEnabled =
          config.value("vits/SentenceSplit", true).toBool();
      m_streamRawReply.clear();
      m_streamDisplayedChinese.clear();
      m_streamSynthCursor = 0;
      m_vitsPendingTexts.clear();
      // 删除暂存的语音文件
      for (QTemporaryFile *file : m_vitsReadyFiles) {
        if (file)
          file->deleteLater();
      }
      m_vitsReadyFiles.clear();
      m_vitsRequestInFlight = false;
      if (m_vitsTempFile) {
        m_vitsTempFile->deleteLater();
        m_vitsTempFile = nullptr;
      }
      if (m_vitsPlayer)
        m_vitsPlayer->stop();
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

  // 刷新历史记录内容
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

    // 显示历史记录窗口动画效果
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

    // 隐藏历史记录动画效果
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

/*移动窗口*/
void Dialog::moveEvent(QMoveEvent *event) {
  if (historyWin && historyWin->isVisible()) {
    QPoint offset = event->pos() - lastPos;
    historyWin->move(historyWin->pos() + offset);
  }
  lastPos = event->pos();
  QWidget::moveEvent(event);
}

/*滚动窗口*/
void Dialog::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().y() > 0)
    handleWheelUp();
  else if (event->angleDelta().y() < 0)
    handleWheelDown();
  QWidget::wheelEvent(event);
}

/*拦截普通的滚动*/
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

/*追加待合成文本*/
void Dialog::VitsGetAndPlay(QString text) {
  qDebug() << "请求合成文本：" << text;

  m_vitsPendingTexts.append(text);
  tryStartNextVitsRequest();
}

/*启动下一个Vits请求*/
void Dialog::tryStartNextVitsRequest() {
  if (!m_vitsManager || !m_vitsPlayer)
    return;
  if (m_vitsRequestInFlight || m_vitsPendingTexts.isEmpty())
    return;

  const QString text = m_vitsPendingTexts.takeFirst();
  if (text.isEmpty())
    return;

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
  qInfo() << "语音合成请求到" << modelAndSpeaker;

  m_vitsRequestInFlight = true;
  QNetworkRequest request(urlString);
  // 发送 GET 请求
  QNetworkReply *reply = m_vitsManager->get(request);
  // 连接信号处理响应
  QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    m_vitsRequestInFlight = false;

    if (reply->error() == QNetworkReply::NoError) {
      QByteArray audioData = reply->readAll();
      if (!audioData.isEmpty()) {
        QTemporaryFile *readyFile =
            new QTemporaryFile(QDir::tempPath() + "/vits_XXXXXX.mp3", this);
        if (readyFile->open()) {
          readyFile->write(audioData);
          readyFile->flush();
          // 合成完成先入就绪队列，播放端按顺序播放
          m_vitsReadyFiles.append(readyFile);
          tryStartNextVitsPlayback();
        } else {
          readyFile->deleteLater();
        }
      }
    }

    reply->deleteLater();
    // 当前请求结束后立即尝试合成下一句，实现“合成前置”。
    tryStartNextVitsRequest();
  });
}

/*启动下一个Vits播放*/
void Dialog::tryStartNextVitsPlayback() {
  if (!m_vitsPlayer)
    return;
  if (m_vitsPlayer->playbackState() != QMediaPlayer::StoppedState)
    return;
  if (m_vitsReadyFiles.isEmpty())
    return;

  if (m_vitsTempFile) {
    m_vitsTempFile->deleteLater();
    m_vitsTempFile = nullptr;
  }

  m_vitsTempFile = m_vitsReadyFiles.takeFirst();
  if (!m_vitsTempFile)
    return;

  // 播放严格串行：只有播放器空闲才取下一句。
  m_vitsPlayer->setSource(QUrl::fromLocalFile(m_vitsTempFile->fileName()));
  m_vitsPlayer->play();
}
