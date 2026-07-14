#include "Pcm16kConverter.h"
#include "SileroVadEngine.h"
#include "SpeechSessionPolicy.h"
#include "VadSegmenter.h"

#include <QAudioFormat>
#include <QFile>
#include <QtEndian>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <cstring>

//语音交互模块的转换、VAD、切段和会话策略测试。
namespace
{
//向测试音频中追加一个 little-endian 16-bit 采样。
void appendPcm16(QByteArray &data, qint16 sample)
{
    const qint16 littleEndian = qToLittleEndian(sample);
    data.append(reinterpret_cast<const char *>(&littleEndian),
                sizeof(littleEndian));
}

//读取测试音频中的一个 little-endian 16-bit 采样。
qint16 readPcm16(const QByteArray &data, int sampleIndex)
{
    qint16 value = 0;
    std::memcpy(&value, data.constData() + sampleIndex * 2, sizeof(value));
    return qFromLittleEndian(value);
}
} //namespace

class SpeechTests : public QObject
{
    Q_OBJECT

  private slots:
    void convertsStereoAndPreservesStreamingFrames();
    void resamples48kTo16k();
    void matchesKeywordsIgnoringFormatting();
    void followsWakeContinuousAndEndFlow();
    void loadsVadAndKeepsSilenceInactive();
    void detectsSpeechFixture();
    void segmentsPreRollSilenceMinimumAndMaximum();
};

void SpeechTests::convertsStereoAndPreservesStreamingFrames()
{
    //验证分块输入不会丢失半个音频帧，并且双声道能正确混音。
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Int16);

    QByteArray input;
    appendPcm16(input, 12000);
    appendPcm16(input, -4000);
    appendPcm16(input, -2000);
    appendPcm16(input, 6000);

    Pcm16kConverter converter;
    converter.reset(format);
    const QByteArray first = converter.process(input.left(3));
    const QByteArray second = converter.process(input.mid(3));

    QVERIFY(first.isEmpty());
    QCOMPARE(second.size(), 4);
    QVERIFY(std::abs(readPcm16(second, 0) - 4000) <= 1);
    QVERIFY(std::abs(readPcm16(second, 1) - 2000) <= 1);
}

void SpeechTests::resamples48kTo16k()
{
    //验证常见的 48 kHz 采集格式能转换为百度要求的 16 kHz。
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QByteArray input;
    for (int index = 0; index < 480; ++index)
        appendPcm16(input, 12000);

    Pcm16kConverter converter;
    converter.reset(format);
    const QByteArray output = converter.process(input);

    QCOMPARE(output.size(), 160 * 2);
    for (int index = 0; index < 160; ++index)
        QVERIFY(std::abs(readPcm16(output, index) - 12000) <= 1);
}

void SpeechTests::matchesKeywordsIgnoringFormatting()
{
    //验证关键词匹配会忽略大小写、空格和标点。
    QVERIFY(SpeechSessionPolicy::matchesKeyword(
        QStringLiteral("Hi， ZC-CHAT！"), QStringList{QStringLiteral("zc chat")}));
    QVERIFY(SpeechSessionPolicy::matchesKeyword(
        QStringLiteral("好的，结束 对话。"),
        QStringList{QStringLiteral("结束对话")}));
    QVERIFY(!SpeechSessionPolicy::matchesKeyword(
        QStringLiteral("继续聊天"), QStringList{QStringLiteral("结束对话")}));
}

void SpeechTests::followsWakeContinuousAndEndFlow()
{
    //验证唤醒前丢弃、唤醒后连续提交以及结束词退出的完整流程。
    SpeechSessionPolicy policy;
    const QStringList wakeWords{QStringLiteral("小助手")};
    const QStringList endWords{QStringLiteral("结束对话")};

    QCOMPARE(static_cast<int>(policy.consumeAutomaticRecognition(
                 QStringLiteral("今天天气"), wakeWords, endWords)),
             static_cast<int>(SpeechSessionPolicy::Decision::Ignore));
    QVERIFY(!policy.isSessionActive());

    QCOMPARE(static_cast<int>(policy.consumeAutomaticRecognition(
                 QStringLiteral("小助手，今天天气怎么样"), wakeWords,
                 endWords)),
             static_cast<int>(SpeechSessionPolicy::Decision::Submit));
    QVERIFY(policy.isSessionActive());

    QCOMPARE(static_cast<int>(policy.consumeAutomaticRecognition(
                 QStringLiteral("再讲一点"), wakeWords, endWords)),
             static_cast<int>(SpeechSessionPolicy::Decision::Submit));

    QCOMPARE(static_cast<int>(policy.consumeAutomaticRecognition(
                 QStringLiteral("结束对话吧"), wakeWords, endWords)),
             static_cast<int>(SpeechSessionPolicy::Decision::SubmitAndEnd));
    QVERIFY(policy.isSessionActive());
    QVERIFY(policy.shouldEndAfterReply());
    QVERIFY(policy.completeOutput());
    QVERIFY(!policy.isSessionActive());
}

void SpeechTests::loadsVadAndKeepsSilenceInactive()
{
    //验证模型资源可加载，静音不触发且 reset 后状态一致。
    QFile model(QStringLiteral(":/models/silero_vad.onnx"));
    QVERIFY2(model.open(QIODevice::ReadOnly), qPrintable(model.errorString()));

    SileroVadEngine vad;
    QString error;
    QVERIFY2(vad.initialize(model.readAll(), &error), qPrintable(error));
    const QByteArray silence(SileroVadEngine::WindowBytes, '\0');
    const float first = vad.processPcm16(silence, &error);
    QVERIFY2(first >= 0.0f, qPrintable(error));
    QVERIFY(first < 0.5f);

    vad.reset();
    const float afterReset = vad.processPcm16(silence, &error);
    QVERIFY2(afterReset >= 0.0f, qPrintable(error));
    QVERIFY(std::abs(first - afterReset) < 0.0001f);
}

void SpeechTests::detectsSpeechFixture()
{
    //使用固定语音夹具验证模型能检测到语音并回到静音。
    QFile model(QStringLiteral(":/models/silero_vad.onnx"));
    QVERIFY2(model.open(QIODevice::ReadOnly), qPrintable(model.errorString()));
    QFile fixture(QStringLiteral(":/fixtures/voice_sample.pcm.qz.b64"));
    QVERIFY2(fixture.open(QIODevice::ReadOnly),
             qPrintable(fixture.errorString()));

    const QByteArray speechPcm =
        qUncompress(QByteArray::fromBase64(fixture.readAll()));
    QVERIFY(!speechPcm.isEmpty());

    SileroVadEngine vad;
    QString error;
    QVERIFY2(vad.initialize(model.readAll(), &error), qPrintable(error));

    float maximumProbability = 0.0f;
    bool returnedToSilence = false;
    QByteArray stream(SileroVadEngine::WindowBytes * 6, '\0');
    stream.append(speechPcm);
    stream.append(QByteArray(SileroVadEngine::WindowBytes * 24, '\0'));
    const int originalSpeechEnd =
        SileroVadEngine::WindowBytes * 6 + speechPcm.size();

    for (int offset = 0; offset < stream.size();
         offset += SileroVadEngine::WindowBytes)
    {
        QByteArray chunk = stream.mid(offset, SileroVadEngine::WindowBytes);
        if (chunk.size() < SileroVadEngine::WindowBytes)
            chunk.append(QByteArray(SileroVadEngine::WindowBytes - chunk.size(),
                                    '\0'));
        const float probability = vad.processPcm16(chunk, &error);
        QVERIFY2(probability >= 0.0f, qPrintable(error));
        maximumProbability = std::max(maximumProbability, probability);
        if (offset > originalSpeechEnd + SileroVadEngine::WindowBytes * 6 &&
            probability < 0.35f)
            returnedToSilence = true;
    }

    QVERIFY2(maximumProbability >= 0.5f,
             qPrintable(QStringLiteral("最高语音概率仅为 %1")
                            .arg(maximumProbability)));
    QVERIFY(returnedToSilence);
}

void SpeechTests::segmentsPreRollSilenceMinimumAndMaximum()
{
    //验证预录、最短语音、结束静音和最长片段限制。
    const QByteArray chunk(VadSegmenter::WindowBytes, '\0');
    VadSegmenter segmenter;

    for (int index = 0; index < 10; ++index)
        QCOMPARE(static_cast<int>(segmenter.process(chunk, 0.0f).event),
                 static_cast<int>(VadSegmenter::Event::None));

    QCOMPARE(static_cast<int>(segmenter.process(chunk, 0.8f).event),
             static_cast<int>(VadSegmenter::Event::Started));
    for (int index = 1; index < 8; ++index)
        segmenter.process(chunk, 0.8f);

    VadSegmenter::Result completed;
    for (int index = 0; index < 22; ++index)
        completed = segmenter.process(chunk, 0.0f);
    QCOMPARE(static_cast<int>(completed.event),
             static_cast<int>(VadSegmenter::Event::Completed));
    QCOMPARE(completed.pcm.size(),
             VadSegmenter::PreRollBytes + 7 * VadSegmenter::WindowBytes +
                 VadSegmenter::RetainedTrailingSilenceSamples * 2);
    QVERIFY(!segmenter.isCapturing());

    segmenter.process(chunk, 0.8f);
    for (int index = 1; index < 7; ++index)
        segmenter.process(chunk, 0.8f);
    VadSegmenter::Result discarded;
    for (int index = 0; index < 22; ++index)
        discarded = segmenter.process(chunk, 0.0f);
    QCOMPARE(static_cast<int>(discarded.event),
             static_cast<int>(VadSegmenter::Event::Discarded));
    QVERIFY(discarded.pcm.isEmpty());

    VadSegmenter::Result maximum;
    for (int index = 0; maximum.event != VadSegmenter::Event::Completed &&
                        index < 2000;
         ++index)
        maximum = segmenter.process(chunk, 0.8f);
    QCOMPARE(static_cast<int>(maximum.event),
             static_cast<int>(VadSegmenter::Event::Completed));
    QCOMPARE(maximum.pcm.size(), VadSegmenter::MaxSegmentBytes);
}

QTEST_APPLESS_MAIN(SpeechTests)

#include "test_speech.moc"
