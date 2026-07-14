#include "VadSegmenter.h"

#include <algorithm>

/*处理一个 VAD 音频窗口*/
VadSegmenter::Result VadSegmenter::process(const QByteArray &pcmChunk,
                                           float speechProbability)
{
    if (pcmChunk.size() != WindowBytes)
        return {};

    if (!m_capturing)
    {
        //未开始说话时保留最近一段音频，避免切掉唤醒词开头。
        m_preRollPcm.append(pcmChunk);
        if (m_preRollPcm.size() > PreRollBytes)
            m_preRollPcm.remove(0, m_preRollPcm.size() - PreRollBytes);
        if (speechProbability < SpeechThreshold)
            return {};

        //达到进入阈值后开始收集，并把预录内容放入片段。
        m_capturing = true;
        m_voicedSamples = WindowSamples;
        m_silenceSamples = 0;
        m_utterancePcm = m_preRollPcm;
        return {Event::Started, {}};
    }

    m_utterancePcm.append(pcmChunk);
    if (speechProbability >= ExitThreshold)
    {
        m_silenceSamples = 0;
        m_voicedSamples += WindowSamples;
    }
    else
    {
        m_silenceSamples += WindowSamples;
    }

    if (m_utterancePcm.size() < MaxSegmentBytes &&
        m_silenceSamples < EndSilenceSamples)
        return {};

    //过短片段通常是噪声或误触发，直接丢弃。
    if (m_voicedSamples < MinimumSpeechSamples)
    {
        reset();
        return {Event::Discarded, {}};
    }

    //最长时长到达后截断，避免单次上传无限增长。
    if (m_utterancePcm.size() > MaxSegmentBytes)
        m_utterancePcm.truncate(MaxSegmentBytes);

    //结束时只保留少量尾部静音，降低上传体积并保留自然停顿。
    const qint64 removableSilence = std::max<qint64>(
        0, m_silenceSamples - RetainedTrailingSilenceSamples);
    const int removableBytes = static_cast<int>(std::min<qint64>(
        removableSilence * 2, m_utterancePcm.size()));
    if (removableBytes > 0)
        m_utterancePcm.chop(removableBytes);

    QByteArray completedPcm = std::move(m_utterancePcm);
    reset();
    return {Event::Completed, std::move(completedPcm)};
}

/*重置切段器*/
void VadSegmenter::reset()
{
    m_preRollPcm.clear();
    m_utterancePcm.clear();
    m_voicedSamples = 0;
    m_silenceSamples = 0;
    m_capturing = false;
}

/*检查是否正在收集语音*/
bool VadSegmenter::isCapturing() const
{
    return m_capturing;
}
