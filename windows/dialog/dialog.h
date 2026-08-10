#ifndef DIALOG_H
#define DIALOG_H

#include "AiProvider.h"
#include "SpeechInteractionController.h"
#include <QByteArray>
#include <QEvent>
#include <QJsonObject>
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
    void contextTokenLimitResolved(const QString &model, int tokenLimit);
    void contextTokenLimitResolutionFailed(const QString &model,
                                           const QString &message);

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
    QStringList activeContextHistory() const;
    bool isHistoryOpen = false;
    QStringList m_contextHistory;
    QString m_contextSummary;
    int m_compactedHistoryCount = 0;

    QString buildUserMessageWithContext(
        const QString &input) const; //构建用户消息，包含上下文
    QString buildUserMessageWithHistory(
        const QString &input, const QStringList &history) const;
    QString buildSystemPrompt() const;
    QString buildContextCompactionSystemPrompt() const;
    void configureAiProvider(AiProvider *provider) const;
    void resetChatProvider();
    void resetContextCompactionProvider();
    void handleChatReplyChunk(const QString &chunk);
    void handleChatReply(const QString &reply);
    void handleChatError(const QString &error);
    bool startAutoContextCompaction(const QString &userInput);
    void handleContextCompactionReply(const QString &reply);
    void handleContextCompactionError(const QString &error);
    void finishContextCompactionWithFallback(const QString &message);
    void clearPendingContextCompaction();
    void sendChatRequest(
        const QString &userInput,
        const QString &preparedRequestMessage = QString());
    void refreshContextTokenLimit();
    void requestModelContextCatalog();
    void applyModelContextCatalog();
    void reportContextTokenLimitResolutionFailure(const QString &message);
    void updateContextTokenDisplay();

    void appendHistoryLine(const QString &line); //添加历史记录行
    void tryStartNextVitsRequest();              //添加到Vits请求
    AiProvider *ai = nullptr;                    //用于AI交互
    AiProvider *m_contextCompactionAi = nullptr;

    QString m_lastUserInput;
    QString m_streamRawReply;
    QString m_streamDisplayedChinese;
    QString m_systemPromptForTokenEstimate;
    QString m_contextTokenLimitSource;
    QString m_pendingCompactionUserInput;
    QString m_pendingCompactionSource;
    QString m_pendingCompactionContextToReplace;
    QString m_pendingCompactionContextPath;
    QString m_aiSessionKey;
    QJsonObject m_modelContextCatalog;
    int m_contextTokenLimit = 0;
    int m_inFlightRequestTokenEstimate = 0;
    int m_pendingCompactionBoundary = 0;
    int m_pendingCompactionHistorySize = 0;
    int m_contextAutoCompactThresholdPercent = 0;
    int m_contextCompactionConsecutiveFailures = 0;
    bool m_modelContextCatalogRequestInFlight = false;
    bool m_contextCompactionInFlight = false;
    bool m_speechInputEnabled = false; //语音输入总开关，用于控制按钮可用性。
    bool m_globalSpeechHotkeyEnabled = false; //全局录音热键是否启用
    bool m_globalSpeechHotkeyPressed = false; //当前热键是否处于按下录音中
    quint32 m_globalSpeechHotkeyNativeKey = 0; //Ela绑定得到的原生按键值
    bool m_streamVitsEnabled = false;
    bool m_streamVitsSentenceSplitEnabled = true;
    int m_streamSynthCursor = 0;
    QStringList m_vitsPendingTexts;
    QList<QTemporaryFile *> m_vitsReadyFiles;
    bool m_vitsRequestInFlight = false;
    bool m_llmReplyInFlight = false; //当前是否有未完成的 LLM 回复。
    QNetworkAccessManager *m_vitsManager = nullptr;
    QNetworkAccessManager *m_modelContextCatalogManager = nullptr;
    QMediaPlayer *m_vitsPlayer = nullptr;
    QAudioOutput *m_vitsAudioOutput = nullptr;
    QTemporaryFile *m_vitsTempFile = nullptr;
    SpeechInteractionController *m_speechController = nullptr; //共享语音控制器。
    //回复链路空闲后恢复自动监听，并刷新麦克风按钮状态。
    void tryStartNextVitsPlayback();
    bool isConversationOutputBusy() const;
    void maybeFinishConversationOutput();
    void updateSpeechButtonState(SpeechInteractionController::State state);
    bool submitCurrentInput();
    void startSpeechRecording();
    void startSpeechRecordingFromHotkey();
    void stopSpeechRecording();
    void releaseSpeechHotkeyResources();
};

#endif //DIALOG_H
