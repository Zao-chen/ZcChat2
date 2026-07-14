#pragma once

#include "Pcm16kConverter.h"
#include "SileroVadEngine.h"
#include "SpeechSessionPolicy.h"
#include "VadSegmenter.h"

#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QStringList>

class QAudioSource;
class QIODevice;
class QMediaDevices;
class QNetworkAccessManager;

//连接麦克风、VAD、百度 ASR 和连续对话策略的语音交互控制器。
class SpeechInteractionController : public QObject
{
    Q_OBJECT

  public:
    enum class State
    {
        Disabled,        //语音输入未启用。
        WaitingForWake,   //自动监听中，等待唤醒词。
        Capturing,        //正在采集一段语音。
        Recognizing,      //正在请求百度语音识别。
        WaitingForReply,  //等待角色生成或播放回复。
        ContinuousReady,  //连续会话中，等待下一轮语音。
        Ending            //已识别结束词，等待最后回复完成。
    };
    Q_ENUM(State)

    struct Config
    {
        bool enabled = false;    //语音模块总开关。
        bool wakeEnabled = false; //是否启用自动唤醒。
        bool autoSend = false;    //手动识别结果是否直接发送。
        QString apiKey;           //百度语音识别 API Key。
        QString secretKey;        //百度语音识别 Secret Key。
        QStringList wakeWords;    //自动监听时使用的唤醒词。
        QStringList endWords;     //连续会话的结束词。
    };

    explicit SpeechInteractionController(QObject *parent = nullptr);
    ~SpeechInteractionController() override;

    //应用语音配置并根据配置重置当前采集和会话。
    void applyConfig(const Config &config);
    //开始或结束手动长按采集。
    void startManualCapture();
    void stopManualCapture();
    //通知控制器角色回复链路是否繁忙。
    void setConversationBusy(bool busy);

    State state() const;
    bool isSessionActive() const;

  signals:
    //autoSubmit 表示识别文本是否应直接提交给角色。
    void recognizedText(const QString &text, bool autoSubmit);
    //状态变化用于更新现有麦克风按钮显示。
    void stateChanged(SpeechInteractionController::State state);
    //连续会话进入或退出时发出。
    void sessionActiveChanged(bool active);
    //报告权限、设备、网络或识别错误。
    void errorOccurred(const QString &message);

  private:
    //区分手动长按和自动 VAD 片段，二者使用不同的唤醒策略。
    enum class CaptureOrigin
    {
        Manual,
        Automatic
    };

    //初始化模型并管理采集、VAD 切段和 ASR 请求。
    void initializeVad();
    //创建、暂停、恢复和销毁共享的麦克风采集器。
    bool ensureCaptureStarted();
    bool ensureMicrophonePermission();
    void stopCapture();
    void suspendCapture();
    void resumeCapture();
    //处理音频输入、VAD 窗口和识别请求。
    void handleAudioReadyRead();
    void processAutomaticPcm(const QByteArray &pcm);
    void processVadChunk(const QByteArray &chunk);
    void resetSegmentState();
    void beginRecognition(const QByteArray &pcm, CaptureOrigin origin);
    void requestAccessToken(quint64 requestId);
    void sendRecognitionRequest(quint64 requestId);
    void handleRecognizedText(const QString &text, CaptureOrigin origin);
    void finishRecognitionWithoutSubmission();
    void returnToListening();
    void setState(State state);
    void resetSession();
    void handleAudioInputsChanged();
    Config m_config;
    State m_state = State::Disabled;
    SileroVadEngine m_vad;
    Pcm16kConverter m_converter;
    SpeechSessionPolicy m_sessionPolicy;
    VadSegmenter m_segmenter;
    QMediaDevices *m_mediaDevices = nullptr;
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioDevice = nullptr;
    QNetworkAccessManager *m_network = nullptr;

    QByteArray m_vadPendingPcm;
    QByteArray m_manualPcm;
    QByteArray m_pendingRecognitionPcm;
    CaptureOrigin m_pendingOrigin = CaptureOrigin::Automatic;
    QString m_accessToken;
    QDateTime m_accessTokenExpiry;
    quint64 m_requestGeneration = 0;

    bool m_manualCapture = false;
    bool m_conversationBusy = false;
    bool m_permissionRequestPending = false;
    bool m_manualStartPending = false;
    bool m_vadReady = false;
};

Q_DECLARE_METATYPE(SpeechInteractionController::State)
