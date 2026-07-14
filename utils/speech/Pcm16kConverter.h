#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QVector>

class Pcm16kConverter
{
  public:
    //重置输入格式及流式转换缓存。
    void reset(const QAudioFormat &inputFormat);
    //将输入音频转换为 16 kHz、16-bit、单声道 PCM。
    QByteArray process(const QByteArray &input);
    //返回当前输入音频格式。
    const QAudioFormat &inputFormat() const;

  private:
    //读取一个采样并归一化到 [-1, 1]。
    float readSample(const char *data) const;
    //将归一化采样量化为 16-bit PCM。
    static qint16 toInt16(float sample);

    QAudioFormat m_inputFormat;
    //保留尚未组成完整音频帧的尾部字节。
    QByteArray m_pendingBytes;
    //重采样时暂存尚未消费的单声道采样。
    QVector<float> m_resampleBuffer;
    //当前输出采样对应的源采样位置。
    double m_sourcePosition = 0.0;
};
