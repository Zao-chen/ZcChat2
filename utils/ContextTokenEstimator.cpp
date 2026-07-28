#include "ContextTokenEstimator.h"

#include <QList>
#include <QLocale>
#include <QtGlobal>

namespace
{
bool isEmojiCodePoint(uint codePoint)
{
    return (codePoint >= 0x1F000 && codePoint <= 0x1FAFF) ||
           (codePoint >= 0x2600 && codePoint <= 0x27BF);
}
} // namespace

namespace ContextTokenEstimator
{
int estimateTextTokens(const QString &text)
{
    if (text.isEmpty())
        return 0;

    // 使用四分之一 token 为单位累计：ASCII 平均约 4 字符/token，
    // 非 ASCII 文本按约 1 字符/token 估算，Emoji 预留更多空间。
    qint64 quarterTokenUnits = 0;
    const QList<uint> codePoints = text.toUcs4();
    for (const uint codePoint : codePoints)
    {
        if (codePoint <= 0x7F)
            quarterTokenUnits += 1;
        else if (isEmojiCodePoint(codePoint))
            quarterTokenUnits += 8;
        else
            quarterTokenUnits += 4;
    }

    return qMax(1, static_cast<int>((quarterTokenUnits + 3) / 4));
}

int estimateChatRequestTokens(const QString &systemPrompt,
                              const QString &userMessage)
{
    // 为消息角色、分隔符和回复起始标记预留少量固定开销。
    int tokens = 2;
    if (!systemPrompt.isEmpty())
        tokens += 4 + estimateTextTokens(systemPrompt);
    if (!userMessage.isEmpty())
        tokens += 4 + estimateTextTokens(userMessage);
    return tokens;
}

int validatedTokenLimit(int tokenLimit)
{
    if (tokenLimit < MinimumContextTokenLimit ||
        tokenLimit > MaximumContextTokenLimit)
        return DefaultContextTokenLimit;
    return tokenLimit;
}

int remainingTokens(int tokenLimit, int estimatedUsedTokens)
{
    return qMax(0, tokenLimit - qMax(0, estimatedUsedTokens));
}

QString formatContextProgressToolTip(int tokenLimit,
                                     int estimatedUsedTokens,
                                     const QString &source,
                                     const QLocale &locale)
{
    const int safeUsed = qMax(0, estimatedUsedTokens);
    const int remaining = remainingTokens(tokenLimit, safeUsed);
    const int overflow = qMax(0, safeUsed - tokenLimit);

    QString tooltip =
        QStringLiteral("本地估算剩余：%1 Token\n"
                       "本地估算已用：%2 Token\n"
                       "上下文上限：%3 Token（%4）")
            .arg(locale.toString(remaining),
                 locale.toString(safeUsed),
                 locale.toString(tokenLimit),
                 source);
    if (overflow > 0)
        tooltip += QStringLiteral("\n已超出：%1 Token")
                       .arg(locale.toString(overflow));
    tooltip += QStringLiteral(
        "\n不同模型的分词方式不同，实际值以模型服务商为准");
    return tooltip;
}
} // namespace ContextTokenEstimator
