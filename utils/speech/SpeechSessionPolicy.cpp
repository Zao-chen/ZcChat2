#include "SpeechSessionPolicy.h"

#include <QRegularExpression>

/*处理一次自动识别结果*/
SpeechSessionPolicy::Decision
SpeechSessionPolicy::consumeAutomaticRecognition(
    const QString &text, const QStringList &wakeWords,
    const QStringList &endWords)
{
    //未进入连续会话时，只有包含唤醒词的整句才会提交。
    if (!m_sessionActive)
    {
        if (!matchesKeyword(text, wakeWords))
            return Decision::Ignore;
        m_sessionActive = true;
    }

    //结束词所在的整句仍然提交，等回复播放完成后再退出会话。
    if (matchesKeyword(text, endWords))
    {
        m_endAfterReply = true;
        return Decision::SubmitAndEnd;
    }
    return Decision::Submit;
}

/*完成最后一条回复并退出会话*/
bool SpeechSessionPolicy::completeOutput()
{
    if (!m_endAfterReply)
        return false;
    m_endAfterReply = false;
    m_sessionActive = false;
    return true;
}

/*重置会话策略*/
void SpeechSessionPolicy::reset()
{
    m_sessionActive = false;
    m_endAfterReply = false;
}

/*检查是否处于连续会话*/
bool SpeechSessionPolicy::isSessionActive() const
{
    return m_sessionActive;
}

/*检查是否等待最后回复完成*/
bool SpeechSessionPolicy::shouldEndAfterReply() const
{
    return m_endAfterReply;
}

/*匹配唤醒词或结束词*/
bool SpeechSessionPolicy::matchesKeyword(const QString &text,
                                         const QStringList &keywords)
{
    const QString normalizedText = normalizeForKeywordMatch(text);
    if (normalizedText.isEmpty())
        return false;
    for (const QString &keyword : keywords)
    {
        const QString normalizedKeyword = normalizeForKeywordMatch(keyword);
        if (!normalizedKeyword.isEmpty() &&
            normalizedText.contains(normalizedKeyword))
            return true;
    }
    return false;
}

/*规范化关键词匹配文本*/
QString SpeechSessionPolicy::normalizeForKeywordMatch(const QString &text)
{
    QString normalized = text.toCaseFolded();
    static const QRegularExpression ignoredCharacters(
        QStringLiteral("[\\s\\p{P}\\p{Z}]+"),
        QRegularExpression::UseUnicodePropertiesOption);
    normalized.remove(ignoredCharacters);
    return normalized;
}
