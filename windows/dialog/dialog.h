#ifndef DIALOG_H
#define DIALOG_H

#include "AiProvider.h"
#include <QEvent>
#include <QMediaCaptureSession>
#include <QMediaRecorder>
#include <QMoveEvent>
#include <QStringList>
#include <QWidget>

class QAudioInput;
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
    void ReloadGeneralConfig();
    void ReloadSpeechInputConfig();
    bool handleSpeechHotkeyEvent(quint32 vkCode, bool isKeyDown, bool isKeyUp);

  private slots:
    void on_pushButton_next_clicked();
    void on_pushButton_history_clicked();
    void on_pushButton_input_pressed();
    void on_pushButton_input_released();
    void on_checkBox_autoInput_toggled(bool checked);
    void rewindToHistoryIndex(int historyIndex);

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
    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;
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
    void stopPendingConversationState();
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
    bool m_isSpeechRecording = false;
    bool m_isSpeechRecognizing = false;
    bool m_globalSpeechHotkeyEnabled = false; //全局录音热键是否启用
    bool m_globalSpeechHotkeyPressed = false; //当前热键是否处于按下录音中
    quint32 m_globalSpeechHotkeyNativeKey = 0; //Ela绑定得到的原生按键值
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
    QMediaRecorder *m_speechRecorder = nullptr;
    QMediaCaptureSession m_speechCaptureSession;
    QAudioInput *m_speechAudioInput = nullptr;
    void tryStartNextVitsPlayback();
    bool submitCurrentInput();
    void startSpeechRecording();
    void startSpeechRecordingFromHotkey();
    void stopSpeechRecording();
    QString speechRecordFilePath() const;
    QString recognizeSpeechFromFile(const QString &filePath);
    QString requestBaiduAccessToken(const QString &apiKey,
                                    const QString &secretKey);
    void releaseSpeechHotkeyResources();
};

#endif //DIALOG_H
