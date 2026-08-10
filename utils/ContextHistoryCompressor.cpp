#include "ContextHistoryCompressor.h"

#include "ContextTokenEstimator.h"

#include <QtGlobal>

namespace
{
constexpr auto kUserPrefix = "用户：";
constexpr auto kSummaryPrefix = "较早对话摘要：";

int validCompactedHistoryCount(const QStringList &history,
                               const QString &summary,
                               int compactedHistoryCount)
{
    if (summary.trimmed().isEmpty() || compactedHistoryCount <= 0 ||
        compactedHistoryCount > history.size())
        return 0;
    return compactedHistoryCount;
}
} // namespace

namespace ContextHistoryCompressor
{
bool Plan::isValid() const
{
    return compactUntil > compactedHistoryCount &&
           !source.trimmed().isEmpty() &&
           !contextToReplace.trimmed().isEmpty();
}

int validatedTriggerPercent(int percent)
{
    if (percent < MinimumTriggerPercent || percent > MaximumTriggerPercent)
        return DefaultTriggerPercent;
    return percent;
}

bool shouldCompress(int estimatedTokens, int contextTokenLimit,
                    int triggerPercent)
{
    if (estimatedTokens <= 0 || contextTokenLimit <= 0)
        return false;

    const qint64 threshold =
        static_cast<qint64>(contextTokenLimit) *
        validatedTriggerPercent(triggerPercent) / 100;
    return estimatedTokens >= threshold;
}

QStringList activeHistory(const QStringList &fullHistory,
                          const QString &summary,
                          int compactedHistoryCount)
{
    const int validCount =
        validCompactedHistoryCount(fullHistory, summary,
                                   compactedHistoryCount);
    if (validCount == 0)
        return fullHistory;

    QStringList result{
        QString::fromUtf8(kSummaryPrefix) + summary.trimmed()};
    result.append(fullHistory.mid(validCount));
    return result;
}

QStringList fastActiveHistory(const QStringList &fullHistory,
                              const QString &summary,
                              int compactedHistoryCount,
                              int compactUntil)
{
    const int validCount =
        validCompactedHistoryCount(fullHistory, summary,
                                   compactedHistoryCount);
    if (compactUntil <= validCount || compactUntil > fullHistory.size())
        return activeHistory(fullHistory, summary,
                             compactedHistoryCount);

    QStringList result;
    if (validCount > 0)
    {
        result.append(
            QString::fromUtf8(kSummaryPrefix) + summary.trimmed());
    }
    result.append(fullHistory.mid(compactUntil));
    return result;
}

Plan createPlan(const QStringList &fullHistory, const QString &summary,
                int compactedHistoryCount, int retainedRecentLines)
{
    Plan plan;
    plan.compactedHistoryCount =
        validCompactedHistoryCount(fullHistory, summary,
                                   compactedHistoryCount);

    const int safeRetainedLines = qMax(2, retainedRecentLines);
    int compactUntil = fullHistory.size() - safeRetainedLines;
    while (compactUntil > plan.compactedHistoryCount &&
           !fullHistory.at(compactUntil)
                .startsWith(QString::fromUtf8(kUserPrefix)))
    {
        --compactUntil;
    }

    //至少压缩一轮完整对话，避免为很小的收益额外调用模型。
    if (compactUntil - plan.compactedHistoryCount < 2)
        return plan;

    QStringList sourceSections;
    QStringList contextToReplace;
    if (plan.compactedHistoryCount > 0)
    {
        sourceSections.append(
            QStringLiteral("已有摘要：\n") + summary.trimmed());
        contextToReplace.append(
            QString::fromUtf8(kSummaryPrefix) + summary.trimmed());
    }
    const QStringList historySlice =
        fullHistory.mid(plan.compactedHistoryCount,
                        compactUntil - plan.compactedHistoryCount);
    sourceSections.append(
        QStringLiteral("需要合并进摘要的较早对话：\n") +
        historySlice.join(QLatin1Char('\n')));
    contextToReplace.append(historySlice);

    plan.compactUntil = compactUntil;
    plan.source = sourceSections.join(QStringLiteral("\n\n"));
    plan.contextToReplace =
        contextToReplace.join(QLatin1Char('\n'));
    return plan;
}

bool isUsefulSummary(const QString &summary,
                     const QString &contextToReplace)
{
    const QString normalizedSummary = summary.trimmed();
    if (normalizedSummary.isEmpty() ||
        contextToReplace.trimmed().isEmpty())
        return false;

    const QString activeSummary =
        QString::fromUtf8(kSummaryPrefix) + normalizedSummary;
    return ContextTokenEstimator::estimateTextTokens(activeSummary) <
           ContextTokenEstimator::estimateTextTokens(contextToReplace);
}
} // namespace ContextHistoryCompressor
