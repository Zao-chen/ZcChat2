#include "Pcm16kConverter.h"

#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace
{
//百度 ASR 和 Silero VAD 统一使用 16 kHz 音频。
constexpr int kTargetSampleRate = 16000;
}

/*重置流式转换状态*/
void Pcm16kConverter::reset(const QAudioFormat &inputFormat)
{
    m_inputFormat = inputFormat;
    m_pendingBytes.clear();
    m_resampleBuffer.clear();
    m_sourcePosition = 0.0;
}

/*转换一个采集音频块*/
QByteArray Pcm16kConverter::process(const QByteArray &input)
{
    if (!m_inputFormat.isValid() || input.isEmpty())
        return QByteArray();

    const int frameBytes = m_inputFormat.bytesPerFrame();
    const int sampleBytes = m_inputFormat.bytesPerSample();
    const int channelCount = m_inputFormat.channelCount();
    const int sampleRate = m_inputFormat.sampleRate();
    if (frameBytes <= 0 || sampleBytes <= 0 || channelCount <= 0 ||
        sampleRate <= 0)
        return QByteArray();

    //输入可能在任意字节位置分块，先拼接上一次未完成的音频帧。
    QByteArray data = m_pendingBytes;
    data.append(input);
    const qsizetype completeBytes = data.size() - data.size() % frameBytes;
    m_pendingBytes = data.mid(completeBytes);
    if (completeBytes <= 0)
        return QByteArray();

    //逐帧混合所有声道，统一转换到浮点范围 [-1, 1]。
    QVector<float> monoSamples;
    monoSamples.reserve(static_cast<int>(completeBytes / frameBytes));
    for (qsizetype offset = 0; offset < completeBytes; offset += frameBytes)
    {
        float mixed = 0.0f;
        const char *frame = data.constData() + offset;
        for (int channel = 0; channel < channelCount; ++channel)
            mixed += readSample(frame + channel * sampleBytes);
        monoSamples.append(std::clamp(mixed / channelCount, -1.0f, 1.0f));
    }

    //采样率不一致时使用流式线性插值，保留跨块所需的尾部采样。
    QVector<float> outputSamples;
    if (sampleRate == kTargetSampleRate)
    {
        outputSamples = std::move(monoSamples);
    }
    else
    {
        m_resampleBuffer += monoSamples;
        const double sourceStep =
            static_cast<double>(sampleRate) / kTargetSampleRate;
        while (static_cast<int>(std::floor(m_sourcePosition)) + 1 <
               m_resampleBuffer.size())
        {
            const int leftIndex =
                static_cast<int>(std::floor(m_sourcePosition));
            const double fraction = m_sourcePosition - leftIndex;
            const float left = m_resampleBuffer.at(leftIndex);
            const float right = m_resampleBuffer.at(leftIndex + 1);
            outputSamples.append(static_cast<float>(
                left + (right - left) * fraction));
            m_sourcePosition += sourceStep;
        }

        const int consumed = static_cast<int>(std::floor(m_sourcePosition));
        if (consumed > 0)
        {
            m_resampleBuffer.remove(0, consumed);
            m_sourcePosition -= consumed;
        }
    }

    //ASR 接口要求 little-endian 的 16-bit PCM 字节流。
    QByteArray output;
    output.resize(outputSamples.size() * static_cast<int>(sizeof(qint16)));
    char *destination = output.data();
    for (qsizetype index = 0; index < outputSamples.size(); ++index)
    {
        const qint16 sample = qToLittleEndian(toInt16(outputSamples.at(index)));
        std::memcpy(destination + index * sizeof(qint16), &sample,
                    sizeof(sample));
    }
    return output;
}

/*获取输入音频格式*/
const QAudioFormat &Pcm16kConverter::inputFormat() const
{
    return m_inputFormat;
}

/*读取并归一化一个采样*/
float Pcm16kConverter::readSample(const char *data) const
{
    switch (m_inputFormat.sampleFormat())
    {
    case QAudioFormat::UInt8:
        return (static_cast<unsigned char>(*data) - 128.0f) / 128.0f;
    case QAudioFormat::Int16:
    {
        qint16 value = 0;
        std::memcpy(&value, data, sizeof(value));
        return static_cast<float>(qFromLittleEndian(value)) / 32768.0f;
    }
    case QAudioFormat::Int32:
    {
        qint32 value = 0;
        std::memcpy(&value, data, sizeof(value));
        return static_cast<float>(qFromLittleEndian(value) /
                                  2147483648.0);
    }
    case QAudioFormat::Float:
    {
        float value = 0.0f;
        std::memcpy(&value, data, sizeof(value));
        return std::isfinite(value) ? std::clamp(value, -1.0f, 1.0f) : 0.0f;
    }
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }
    return 0.0f;
}

/*将浮点采样量化为 16-bit PCM*/
qint16 Pcm16kConverter::toInt16(float sample)
{
    const float bounded = std::clamp(sample, -1.0f, 1.0f);
    if (bounded <= -1.0f)
        return std::numeric_limits<qint16>::min();
    return static_cast<qint16>(
        std::lround(bounded * std::numeric_limits<qint16>::max()));
}
