#pragma once

#include <QByteArray>

//根据 VAD 概率把固定长度音频块切分为完整语音片段。
class VadSegmenter
{
  public:
    enum class Event
    {
        None,       //当前窗口未产生切段事件。
        Started,    //检测到语音起点。
        Completed,  //语音片段已完成，可以提交识别。
        Discarded   //片段过短，已丢弃。
    };

    struct Result
    {
        Event event = Event::None;
        QByteArray pcm;
    };

    //语音切段使用的固定采样率和窗口大小。
    static constexpr int SampleRate = 16000;
    static constexpr int WindowSamples = 512;
    static constexpr int WindowBytes = WindowSamples * 2;
    //进入语音和退出语音使用不同阈值，避免边界抖动。
    static constexpr float SpeechThreshold = 0.5f;
    static constexpr float ExitThreshold = 0.35f;
    //语音片段的时长、预录和最长长度限制。
    static constexpr int MinimumSpeechSamples = SampleRate * 250 / 1000;
    static constexpr int EndSilenceSamples = SampleRate * 700 / 1000;
    static constexpr int RetainedTrailingSilenceSamples =
        SampleRate * 200 / 1000;
    static constexpr int PreRollBytes = SampleRate * 300 / 1000 * 2;
    static constexpr int MaxSegmentBytes = SampleRate * 55 * 2;

    //输入一个音频块并返回切段事件。
    Result process(const QByteArray &pcmChunk, float speechProbability);
    //清空当前片段和预录缓存。
    void reset();
    //判断当前是否正在收集语音片段。
    bool isCapturing() const;

  private:
    //保存语音开始前的短暂音频，避免截掉起始音节。
    QByteArray m_preRollPcm;
    //当前正在收集的语音片段。
    QByteArray m_utterancePcm;
    //已确认的语音采样数和连续静音采样数。
    qint64 m_voicedSamples = 0;
    qint64 m_silenceSamples = 0;
    bool m_capturing = false;
};
