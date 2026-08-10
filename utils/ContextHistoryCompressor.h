#ifndef CONTEXTHISTORYCOMPRESSOR_H
#define CONTEXTHISTORYCOMPRESSOR_H

#include <QString>
#include <QStringList>

namespace ContextHistoryCompressor
{
inline constexpr int DefaultTriggerPercent = 80;
inline constexpr int MinimumTriggerPercent = 50;
inline constexpr int MaximumTriggerPercent = 95;
inline constexpr int RetainedRecentLines = 4;

struct Plan
{
    int compactedHistoryCount = 0;
    int compactUntil = 0;
    QString source;
    QString contextToReplace;

    bool isValid() const;
};

int validatedTriggerPercent(int percent);
bool shouldCompress(int estimatedTokens, int contextTokenLimit,
                    int triggerPercent);

QStringList activeHistory(const QStringList &fullHistory,
                          const QString &summary,
                          int compactedHistoryCount);
QStringList fastActiveHistory(const QStringList &fullHistory,
                              const QString &summary,
                              int compactedHistoryCount,
                              int compactUntil);
Plan createPlan(const QStringList &fullHistory, const QString &summary,
                int compactedHistoryCount,
                int retainedRecentLines = RetainedRecentLines);
bool isUsefulSummary(const QString &summary,
                     const QString &contextToReplace);
} // namespace ContextHistoryCompressor

#endif // CONTEXTHISTORYCOMPRESSOR_H
