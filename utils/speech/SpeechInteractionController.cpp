#include "SpeechInteractionController.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QCoreApplication>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPermissions>
#include <QUrlQuery>
#include <QUuid>

#include <algorithm>

/*初始化语音交互控制器*/
SpeechInteractionController::SpeechInteractionController(QObject *parent)
    : QObject(parent), m_mediaDevices(new QMediaDevices(this)),
      m_network(new QNetworkAccessManager(this))
{
    qRegisterMetaType<SpeechInteractionController::State>();
    connect(m_mediaDevices, &QMediaDevices::audioInputsChanged, this,
            &SpeechInteractionController::handleAudioInputsChanged);
    initializeVad();
}

/*释放采集资源*/
SpeechInteractionController::~SpeechInteractionController()
{
    stopCapture();
}

/*应用语音配置并重置当前会话*/
void SpeechInteractionController::applyConfig(const Config &config)
{
    //凭据变化后必须丢弃旧 Token，避免使用其他账号的缓存。
    const bool credentialsChanged =
        config.apiKey != m_config.apiKey || config.secretKey != m_config.secretKey;
    ++m_requestGeneration;
    stopCapture();
    resetSegmentState();
    resetSession();
    m_config = config;
    if (credentialsChanged)
    {
        m_accessToken.clear();
        m_accessTokenExpiry = QDateTime();
    }


    //总开关关闭时不申请设备，也不保留连续会话。
    if (!m_config.enabled)
    {
        setState(State::Disabled);
        return;
    }

    //自动唤醒需要模型和至少一个唤醒词。
    if (m_config.wakeEnabled)
    {
        if (m_config.wakeWords.isEmpty())
        {
            qWarning() << "controller.config.invalid"
                                 << "reason" << "missing_wake_words";
            emit errorOccurred(QStringLiteral("请先选择角色或配置语音唤醒词"));
            setState(State::Disabled);
            return;
        }
        if (!m_vadReady)
        {
            qWarning() << "controller.config.invalid"
                                 << "reason" << "vad_unavailable";
            emit errorOccurred(QStringLiteral("Silero VAD 初始化失败，语音唤醒不可用"));
            setState(State::Disabled);
            return;
        }
        setState(State::WaitingForWake);
        ensureCaptureStarted();
        return;
    }

#ifdef Q_OS_MACOS
    //启用语音输入时提前申请权限，手动采集开始前仍会再次检查。
    ensureMicrophonePermission();
#endif
    setState(State::Disabled);
}

/*开始手动长按采集*/
void SpeechInteractionController::startManualCapture()
{
    if (!m_config.enabled || m_conversationBusy ||
        m_state == State::Recognizing || m_manualCapture ||
        m_segmenter.isCapturing())
        return;

    m_manualStartPending = true;
    if (!ensureCaptureStarted())
        return;

    m_manualStartPending = false;
    resetSegmentState();
    m_manualCapture = true;
    setState(State::Capturing);
}

/*结束手动长按采集并提交识别*/
void SpeechInteractionController::stopManualCapture()
{
    m_manualStartPending = false;
    if (!m_manualCapture)
        return;

    m_manualCapture = false;
    const QByteArray pcm = m_manualPcm;
    m_manualPcm.clear();
    if (!m_config.wakeEnabled)
        stopCapture();
    else
        suspendCapture();

    if (pcm.isEmpty())
    {
        returnToListening();
        return;
    }
    beginRecognition(pcm, CaptureOrigin::Manual);
}

/*同步角色回复链路的忙闲状态*/
void SpeechInteractionController::setConversationBusy(bool busy)
{
    m_conversationBusy = busy;
    if (busy)
    {
        suspendCapture();
        setState(m_sessionPolicy.shouldEndAfterReply() ? State::Ending
                                                       : State::WaitingForReply);
        return;
    }

    if (m_sessionPolicy.completeOutput())
    {
        emit sessionActiveChanged(false);
    }
    returnToListening();
}

/*获取当前语音交互状态*/
SpeechInteractionController::State SpeechInteractionController::state() const
{
    return m_state;
}

/*获取当前是否处于连续会话*/
bool SpeechInteractionController::isSessionActive() const
{
    return m_sessionPolicy.isSessionActive();
}

/*从 Qt 资源加载 Silero VAD 模型*/
void SpeechInteractionController::initializeVad()
{
    QFile model(QStringLiteral(":/models/silero_vad.onnx"));
    if (!model.open(QIODevice::ReadOnly))
    {
        m_vadReady = false;
        qCritical() << "vad.model.open.failed";
        return;
    }

    QString error;
    m_vadReady = m_vad.initialize(model.readAll(), &error);
    if (!m_vadReady && !error.isEmpty())
    {
        qCritical() << "vad.initialize.failed" << error;
        emit errorOccurred(QStringLiteral("Silero VAD 初始化失败：") + error);
    }
}

/*创建并启动共享麦克风采集器*/
bool SpeechInteractionController::ensureCaptureStarted()
{
    if (m_audioSource && m_audioDevice)
    {
        resumeCapture();
        return true;
    }

    if (!ensureMicrophonePermission())
        return false;

    //手动输入和自动唤醒共用同一个默认麦克风设备。
    const QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();
    if (inputDevice.isNull())
    {
        qWarning() << "capture.start.failed" << "reason" << "no_microphone";
        emit errorOccurred(QStringLiteral("未检测到可用麦克风"));
        return false;
    }

    QAudioFormat desiredFormat;
    desiredFormat.setSampleRate(16000);
    desiredFormat.setChannelCount(1);
    desiredFormat.setSampleFormat(QAudioFormat::Int16);
    //优先直接采集目标格式，否则交给转换器做声道和采样率转换。
    const QAudioFormat captureFormat =
        inputDevice.isFormatSupported(desiredFormat)
            ? desiredFormat
            : inputDevice.preferredFormat();
    if (!captureFormat.isValid())
    {
        qWarning() << "capture.start.failed"
                             << "reason" << "invalid_audio_format";
        emit errorOccurred(QStringLiteral("麦克风音频格式无效"));
        return false;
    }

    m_converter.reset(captureFormat);
    m_audioSource = new QAudioSource(inputDevice, captureFormat, this);
    m_audioDevice = m_audioSource->start();
    if (!m_audioDevice)
    {
        qWarning() << "capture.start.failed"
                             << "reason" << "audio_source_start_failed";
        emit errorOccurred(QStringLiteral("无法启动麦克风采集"));
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        return false;
    }
    connect(m_audioDevice, &QIODevice::readyRead, this,
            &SpeechInteractionController::handleAudioReadyRead);
    return true;
}

/*检查并请求 macOS 麦克风权限*/
bool SpeechInteractionController::ensureMicrophonePermission()
{
#ifdef Q_OS_MACOS
    QCoreApplication *application = QCoreApplication::instance();
    if (!application)
        return false;

    const QMicrophonePermission permission;
    const Qt::PermissionStatus status = application->checkPermission(permission);
    if (status == Qt::PermissionStatus::Denied)
    {
        emit errorOccurred(
            QStringLiteral("麦克风权限未开启，请在系统设置中允许 ZcChat2 使用麦克风"));
        return false;
    }
    if (status == Qt::PermissionStatus::Undetermined)
    {
        if (!m_permissionRequestPending)
        {
            m_permissionRequestPending = true;
            application->requestPermission(
                permission, this,
                [this](const QPermission &result)
                {
                    m_permissionRequestPending = false;
                    if (result.status() != Qt::PermissionStatus::Granted)
                    {
                        m_manualStartPending = false;
                        emit errorOccurred(QStringLiteral(
                            "麦克风权限未开启，请在系统设置中允许 ZcChat2 使用麦克风"));
                        return;
                    }
                    if (m_manualStartPending)
                        startManualCapture();
                    else if (m_config.enabled && m_config.wakeEnabled)
                        ensureCaptureStarted();
                });
        }
        return false;
    }
#endif
    return true;
}

/*停止并释放麦克风采集器*/
void SpeechInteractionController::stopCapture()
{
    if (m_audioSource)
    {
        m_audioSource->stop();
        m_audioSource->deleteLater();
    }
    m_audioSource = nullptr;
    m_audioDevice = nullptr;
}

/*暂停采集但保留设备，等待回复链路空闲*/
void SpeechInteractionController::suspendCapture()
{
    if (m_audioSource)
        m_audioSource->suspend();
}

/*恢复已创建的麦克风采集器*/
void SpeechInteractionController::resumeCapture()
{
    if (m_audioSource)
        m_audioSource->resume();
}

/*处理麦克风可读数据*/
void SpeechInteractionController::handleAudioReadyRead()
{
    if (!m_audioDevice)
        return;
    const QByteArray pcm = m_converter.process(m_audioDevice->readAll());
    if (pcm.isEmpty())
        return;

    //手动长按直接缓存整段 PCM，松开后一次性提交百度识别。
    if (m_manualCapture)
    {
        const int remaining =
            VadSegmenter::MaxSegmentBytes - m_manualPcm.size();
        if (remaining > 0)
            m_manualPcm.append(pcm.left(remaining));
        if (m_manualPcm.size() >= VadSegmenter::MaxSegmentBytes)
            stopManualCapture();
        return;
    }

    //回复生成或播放期间暂停自动监听，避免采集到角色自己的声音。
    if (m_config.wakeEnabled && !m_conversationBusy &&
        m_state != State::Recognizing)
        processAutomaticPcm(pcm);
}

/*把自动采集数据按模型窗口切分*/
void SpeechInteractionController::processAutomaticPcm(const QByteArray &pcm)
{
    m_vadPendingPcm.append(pcm);
    while (m_vadPendingPcm.size() >= SileroVadEngine::WindowBytes)
    {
        const QByteArray chunk =
            m_vadPendingPcm.left(SileroVadEngine::WindowBytes);
        m_vadPendingPcm.remove(0, SileroVadEngine::WindowBytes);
        processVadChunk(chunk);
        if (m_state == State::Recognizing)
            break;
    }
}

/*处理一个 VAD 窗口并推进切段状态*/
void SpeechInteractionController::processVadChunk(const QByteArray &chunk)
{
    QString error;
    const float probability = m_vad.processPcm16(chunk, &error);
    if (probability < 0.0f)
    {
        qCritical() << "vad.inference.failed" << error;
        emit errorOccurred(QStringLiteral("Silero VAD 推理失败：") + error);
        m_vadReady = false;
        stopCapture();
        resetSegmentState();
        resetSession();
        setState(State::Disabled);
        return;
    }

    const VadSegmenter::Result segment = m_segmenter.process(chunk, probability);
    switch (segment.event)
    {
    case VadSegmenter::Event::None:
        break;
    case VadSegmenter::Event::Started:
        //只在检测到语音起点后更新按钮状态。
        setState(State::Capturing);
        break;
    case VadSegmenter::Event::Discarded:
        returnToListening();
        break;
    case VadSegmenter::Event::Completed:
        //切段完成后暂停麦克风，等待 ASR 请求结束。
        m_vadPendingPcm.clear();
        m_vad.reset();
        beginRecognition(segment.pcm, CaptureOrigin::Automatic);
        break;
    }
}

/*清空 VAD 和当前录音片段*/
void SpeechInteractionController::resetSegmentState()
{
    m_vadPendingPcm.clear();
    m_segmenter.reset();
    m_manualPcm.clear();
    m_vad.reset();
}

/*开始一次百度语音识别*/
void SpeechInteractionController::beginRecognition(const QByteArray &pcm,
                                                   CaptureOrigin origin)
{
    if (pcm.isEmpty())
    {
        finishRecognitionWithoutSubmission();
        return;
    }

    suspendCapture();
    setState(State::Recognizing);
    m_pendingRecognitionPcm = pcm;
    m_pendingOrigin = origin;
    const quint64 requestId = ++m_requestGeneration;
    qInfo() << "recognition.started"
                      << "request_id" << requestId
                      << "origin"
                      << (origin == CaptureOrigin::Manual ? "manual" : "automatic")
                      << "pcm_bytes" << pcm.size()
                      << "cached_token" << (!m_accessToken.isEmpty() &&
                             QDateTime::currentDateTimeUtc() < m_accessTokenExpiry);

    if (m_config.apiKey.trimmed().isEmpty() ||
        m_config.secretKey.trimmed().isEmpty())
    {
        qWarning() << "recognition.config.incomplete"
                             << "request_id" << requestId;
        emit errorOccurred(QStringLiteral("百度语音识别配置不完整"));
        finishRecognitionWithoutSubmission();
        return;
    }

    //Token 在有效期内复用，否则先异步获取新 Token。
    if (!m_accessToken.isEmpty() &&
        QDateTime::currentDateTimeUtc() < m_accessTokenExpiry)
        sendRecognitionRequest(requestId);
    else
        requestAccessToken(requestId);
}

/*异步获取百度 Access Token*/
void SpeechInteractionController::requestAccessToken(quint64 requestId)
{
    qDebug() << "recognition.token.started" << "request_id" << requestId;
    QUrl url(QStringLiteral("https://aip.baidubce.com/oauth/2.0/token"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("grant_type"),
                       QStringLiteral("client_credentials"));
    query.addQueryItem(QStringLiteral("client_id"), m_config.apiKey.trimmed());
    query.addQueryItem(QStringLiteral("client_secret"),
                       m_config.secretKey.trimmed());
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    QNetworkReply *reply = m_network->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]()
            {
                const QByteArray body = reply->readAll();
                const bool validRequest = requestId == m_requestGeneration;
                const bool succeeded = reply->error() == QNetworkReply::NoError;
                const QString networkError = reply->errorString();
                reply->deleteLater();
                //配置或设备变化后，旧请求的结果不能再影响当前状态。
                if (!validRequest)
                {
                    return;
                }
                if (!succeeded)
                {
                    qWarning() << "recognition.token.failed"
                                         << "request_id" << requestId
                                         << networkError;
                    emit errorOccurred(QStringLiteral("百度 Token 获取失败：") +
                                       networkError);
                    finishRecognitionWithoutSubmission();
                    return;
                }

                const QJsonObject object =
                    QJsonDocument::fromJson(body).object();
                m_accessToken =
                    object.value(QStringLiteral("access_token")).toString();
                const int expiresIn =
                    object.value(QStringLiteral("expires_in")).toInt(2592000);
                if (m_accessToken.isEmpty())
                {
                    qWarning() << "recognition.token.invalid_response"
                                         << "request_id" << requestId;
                    emit errorOccurred(QStringLiteral("百度 Token 响应无效"));
                    finishRecognitionWithoutSubmission();
                    return;
                }
                m_accessTokenExpiry = QDateTime::currentDateTimeUtc().addSecs(
                    std::max(60, expiresIn - 60));
                sendRecognitionRequest(requestId); });
}

/*异步提交 PCM 到百度语音识别接口*/
void SpeechInteractionController::sendRecognitionRequest(quint64 requestId)
{
    qDebug() << "recognition.request.sent"
                       << "request_id" << requestId
                       << "pcm_bytes" << m_pendingRecognitionPcm.size();
    QJsonObject payload{
        {QStringLiteral("format"), QStringLiteral("pcm")},
        {QStringLiteral("rate"), 16000},
        {QStringLiteral("channel"), 1},
        {QStringLiteral("token"), m_accessToken},
        {QStringLiteral("cuid"),
         QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("speech"),
         QString::fromLatin1(m_pendingRecognitionPcm.toBase64())},
        {QStringLiteral("len"), m_pendingRecognitionPcm.size()},
    };

    QNetworkRequest request(
        QUrl(QStringLiteral("https://vop.baidu.com/server_api")));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    QNetworkReply *reply = m_network->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]()
            {
                const QByteArray body = reply->readAll();
                const bool validRequest = requestId == m_requestGeneration;
                const bool succeeded = reply->error() == QNetworkReply::NoError;
                const QString networkError = reply->errorString();
                reply->deleteLater();
                if (!validRequest)
                {
                    return;
                }
                if (!succeeded)
                {
                    qWarning() << "recognition.request.failed"
                                         << "request_id" << requestId
                                         << networkError;
                    emit errorOccurred(QStringLiteral("百度语音识别失败：") +
                                       networkError);
                    finishRecognitionWithoutSubmission();
                    return;
                }

                const QJsonObject object =
                    QJsonDocument::fromJson(body).object();
                const int errorNumber =
                    object.value(QStringLiteral("err_no")).toInt();
                if (errorNumber != 0)
                {
                    qWarning() << "recognition.api.failed"
                                         << "request_id" << requestId
                                         << "error_code" << errorNumber;
                    emit errorOccurred(
                        QStringLiteral("百度语音识别失败（%1）：%2")
                            .arg(errorNumber)
                            .arg(object.value(QStringLiteral("err_msg"))
                                     .toString()));
                    m_pendingRecognitionPcm.clear();
                    finishRecognitionWithoutSubmission();
                    return;
                }
                const QJsonArray result =
                    object.value(QStringLiteral("result")).toArray();
                //发送给角色的是 ASR 返回的完整句子，不删除唤醒词或结束词。
                const QString text = result.isEmpty()
                                         ? QString()
                                         : result.first().toString().trimmed();
                const CaptureOrigin origin = m_pendingOrigin;
                m_pendingRecognitionPcm.clear();
                if (text.isEmpty())
                {
                    if (origin == CaptureOrigin::Manual)
                        emit errorOccurred(QStringLiteral("没有识别到有效语音"));
                    finishRecognitionWithoutSubmission();
                    return;
                }
                qInfo() << "recognition.completed"
                                  << "request_id" << requestId
                                  << "characters" << text.size();
                handleRecognizedText(text, origin); });
}

/*处理一条识别文本并推进会话策略*/
void SpeechInteractionController::handleRecognizedText(
    const QString &text, CaptureOrigin origin)
{
    //手动输入绕过唤醒词，但仍遵守“直接发送”配置。
    if (origin == CaptureOrigin::Manual)
    {
        if (m_config.autoSend)
        {
            suspendCapture();
            setState(State::WaitingForReply);
        }
        else
        {
            returnToListening();
        }
        emit recognizedText(text, m_config.autoSend);
        return;
    }

    //自动输入交给会话策略判断是否忽略、提交或提交后结束。
    const bool wasSessionActive = m_sessionPolicy.isSessionActive();
    const SpeechSessionPolicy::Decision decision =
        m_sessionPolicy.consumeAutomaticRecognition(
            text, m_config.wakeWords, m_config.endWords);
    if (decision == SpeechSessionPolicy::Decision::Ignore)
    {
        finishRecognitionWithoutSubmission();
        return;
    }
    if (!wasSessionActive && m_sessionPolicy.isSessionActive())
        emit sessionActiveChanged(true);

    suspendCapture();
    setState(decision == SpeechSessionPolicy::Decision::SubmitAndEnd
                 ? State::Ending
                 : State::WaitingForReply);
    emit recognizedText(text, true);
}

/*识别失败或无效时恢复监听*/
void SpeechInteractionController::finishRecognitionWithoutSubmission()
{
    m_pendingRecognitionPcm.clear();
    returnToListening();
}

/*根据当前配置恢复等待唤醒或连续对话监听*/
void SpeechInteractionController::returnToListening()
{
    resetSegmentState();
    if (!m_config.enabled)
    {
        stopCapture();
        setState(State::Disabled);
        return;
    }
    if (m_conversationBusy)
    {
        suspendCapture();
        setState(m_sessionPolicy.shouldEndAfterReply() ? State::Ending
                                                       : State::WaitingForReply);
        return;
    }
    if (!m_config.wakeEnabled)
    {
        stopCapture();
        setState(State::Disabled);
        return;
    }
    if (!m_vadReady || m_config.wakeWords.isEmpty())
    {
        stopCapture();
        setState(State::Disabled);
        return;
    }

    setState(m_sessionPolicy.isSessionActive() ? State::ContinuousReady
                                               : State::WaitingForWake);
    if (ensureCaptureStarted())
        resumeCapture();
}

/*更新状态并通知界面*/
void SpeechInteractionController::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

/*重置连续会话和手动采集标记*/
void SpeechInteractionController::resetSession()
{
    m_conversationBusy = false;
    m_manualCapture = false;
    m_manualStartPending = false;
    const bool wasSessionActive = m_sessionPolicy.isSessionActive();
    m_sessionPolicy.reset();
    if (wasSessionActive)
        emit sessionActiveChanged(false);
}

/*处理默认麦克风切换或断开*/
void SpeechInteractionController::handleAudioInputsChanged()
{
    ++m_requestGeneration;
    stopCapture();
    resetSegmentState();
    resetSession();
    if (!m_config.enabled || !m_config.wakeEnabled || !m_vadReady ||
        m_config.wakeWords.isEmpty())
    {
        setState(State::Disabled);
        return;
    }
    if (QMediaDevices::defaultAudioInput().isNull())
    {
        setState(State::Disabled);
        emit errorOccurred(QStringLiteral("麦克风已断开"));
        return;
    }
    setState(State::WaitingForWake);
    ensureCaptureStarted();
}
