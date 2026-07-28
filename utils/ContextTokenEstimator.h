#ifndef CONTEXTTOKENESTIMATOR_H
#define CONTEXTTOKENESTIMATOR_H

#include <QString>

class QLocale;

namespace ContextTokenEstimator
{
inline constexpr int DefaultContextTokenLimit = 65536;
inline constexpr int MinimumContextTokenLimit = 1024;
inline constexpr int ContextTokenLimitStep = MinimumContextTokenLimit;
// 为超长上下文模型和手动配置预留空间。
inline constexpr int MaximumContextTokenLimit = 10000000;

// 不同模型的分词器并不统一，这里只提供适合界面提示的本地估算值。
int estimateTextTokens(const QString &text);
int estimateChatRequestTokens(const QString &systemPrompt,
                              const QString &userMessage);
// 配置值越界时回退到默认上限。
int validatedTokenLimit(int tokenLimit);
int remainingTokens(int tokenLimit, int estimatedUsedTokens);
QString formatContextProgressToolTip(int tokenLimit,
                                     int estimatedUsedTokens,
                                     const QString &source,
                                     const QLocale &locale);
} // namespace ContextTokenEstimator

#endif // CONTEXTTOKENESTIMATOR_H
