#pragma once

#include <QString>
#include <QStringList>

//管理唤醒词、连续对话和结束词之间的会话策略。
class SpeechSessionPolicy
{
  public:
    enum class Decision
    {
        Ignore,        //未命中唤醒词，丢弃识别结果。
        Submit,        //提交识别结果并保持会话。
        SubmitAndEnd   //提交识别结果，回复完成后退出会话。
    };

    //消费一次自动识别结果并返回后续提交动作。
    Decision consumeAutomaticRecognition(const QString &text,
                                         const QStringList &wakeWords,
                                         const QStringList &endWords);
    //在最后一条回复播放完成后结束待退出会话。
    bool completeOutput();
    //清空连续会话状态。
    void reset();

    //返回当前是否已经进入连续对话。
    bool isSessionActive() const;
    //返回是否需要在当前回复完成后退出会话。
    bool shouldEndAfterReply() const;

    //对识别文本和关键词做规范化后进行包含匹配。
    static bool matchesKeyword(const QString &text,
                               const QStringList &keywords);
    //忽略大小写、空白和常见标点。
    static QString normalizeForKeywordMatch(const QString &text);

  private:
    bool m_sessionActive = false;
    bool m_endAfterReply = false;
};
