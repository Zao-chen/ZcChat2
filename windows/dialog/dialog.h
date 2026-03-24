#ifndef DIALOG_H
#define DIALOG_H

#include "AiProvider.h"
#include <QEvent>
#include <QMoveEvent>
#include <QStringList>
#include <QWidget>

class QAudioOutput;
class QMediaPlayer;
class QNetworkAccessManager;
class QTemporaryFile;

namespace Ui
{
class Dialog;
}

class history;

class Dialog : public QWidget
{
    Q_OBJECT

  public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

  public slots:
    void ToggleVisible();
    void VitsGetAndPlay(QString text);

  private slots:
    void on_pushButton_next_clicked();
    void on_pushButton_history_clicked();

  signals:
    void requestSetCharTachie(QString TachieName);

  public slots:
    void ReloadAIConfig();

  private:
    /*初始化*/
    virtual void paintEvent(QPaintEvent *event) override;
    Ui::Dialog *ui = nullptr;
    history *historyWin = nullptr;
    /*按键事件*/
    //鼠标
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    QPoint lastPos;  //记录鼠标位置
    QList<int> keys; //按键按键获取
    /*主逻辑*/
    void initWindow();
    //历史
    void handleWheelUp();
    void handleWheelDown();
    void loadContextHistory(); //加载上下文历史
    void saveContextHistory() const;
    bool isHistoryOpen = false;
    QStringList m_contextHistory;

    QString buildUserMessageWithContext(
        const QString &input) const; //构建用户消息，包含上下文

    void appendHistoryLine(const QString &line); //添加历史记录行
    void tryStartNextVitsRequest();              //添加到Vits请求
    AiProvider *ai = nullptr;                    //用于AI交互

    QString m_lastUserInput;
    QString m_streamRawReply;
    QString m_streamDisplayedChinese;
    bool m_streamVitsEnabled = false;
    bool m_streamVitsSentenceSplitEnabled = true;
    int m_streamSynthCursor = 0;
    QStringList m_vitsPendingTexts;
    QList<QTemporaryFile *> m_vitsReadyFiles;
    bool m_vitsRequestInFlight = false;
    QNetworkAccessManager *m_vitsManager = nullptr;
    QMediaPlayer *m_vitsPlayer = nullptr;
    QAudioOutput *m_vitsAudioOutput = nullptr;
    QTemporaryFile *m_vitsTempFile = nullptr;
    void tryStartNextVitsPlayback();
};

#endif //DIALOG_H
