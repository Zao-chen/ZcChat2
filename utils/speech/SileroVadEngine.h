#pragma once

#include <QByteArray>
#include <QString>

#include <memory>

//使用 ONNX Runtime 执行 Silero VAD 的流式推理。
class SileroVadEngine
{
  public:
    SileroVadEngine();
    ~SileroVadEngine();

    //从内存中的 ONNX 模型初始化推理会话。
    bool initialize(const QByteArray &modelData, QString *errorMessage = nullptr);
    //判断模型会话是否已经准备完成。
    bool isReady() const;
    //处理一个固定长度的 16 kHz、16-bit、单声道 PCM 音频块。
    float processPcm16(const QByteArray &pcmChunk,
                       QString *errorMessage = nullptr);
    //清空模型上下文和循环状态。
    void reset();

    //Silero VAD 模型要求的输入采样率和窗口大小。
    static constexpr int SampleRate = 16000;
    static constexpr int WindowSamples = 512;
    static constexpr int WindowBytes = WindowSamples * 2;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
