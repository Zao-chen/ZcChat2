#include "ContextHistoryCompressor.h"

#include <QtTest>

class ContextHistoryCompressorTests : public QObject
{
    Q_OBJECT

  private slots:
    void validatesTriggerPercentage();
    void triggersAtConfiguredUsage();
    void keepsFullHistoryWhenCheckpointIsInvalid();
    void buildsActiveHistoryFromSummaryAndRecentLines();
    void buildsFastHistoryWithoutMutatingFullHistory();
    void createsPlanOnConversationBoundary();
    void skipsPlanWithoutEnoughOldHistory();
    void rejectsSummaryThatDoesNotReduceContext();
};

void ContextHistoryCompressorTests::validatesTriggerPercentage()
{
    using namespace ContextHistoryCompressor;
    QCOMPARE(validatedTriggerPercent(50), 50);
    QCOMPARE(validatedTriggerPercent(95), 95);
    QCOMPARE(validatedTriggerPercent(49), DefaultTriggerPercent);
    QCOMPARE(validatedTriggerPercent(96), DefaultTriggerPercent);
}

void ContextHistoryCompressorTests::triggersAtConfiguredUsage()
{
    using namespace ContextHistoryCompressor;
    QVERIFY(!shouldCompress(799, 1000, 80));
    QVERIFY(shouldCompress(800, 1000, 80));
    QVERIFY(shouldCompress(900, 1000, 80));
}

void ContextHistoryCompressorTests::keepsFullHistoryWhenCheckpointIsInvalid()
{
    using namespace ContextHistoryCompressor;
    const QStringList history{QStringLiteral("用户：你好"),
                              QStringLiteral("角色：你好呀")};
    QCOMPARE(activeHistory(history, QString(), 1), history);
    QCOMPARE(activeHistory(history, QStringLiteral("旧摘要"), 3), history);
}

void ContextHistoryCompressorTests::buildsActiveHistoryFromSummaryAndRecentLines()
{
    using namespace ContextHistoryCompressor;
    const QStringList history{
        QStringLiteral("用户：第一问"), QStringLiteral("角色：第一答"),
        QStringLiteral("用户：第二问"), QStringLiteral("角色：第二答"),
        QStringLiteral("用户：第三问"), QStringLiteral("角色：第三答")};

    const QStringList active =
        activeHistory(history, QStringLiteral("已经讨论过第一问"), 2);
    QCOMPARE(active.size(), 5);
    QCOMPARE(active.first(), QStringLiteral("较早对话摘要：已经讨论过第一问"));
    QCOMPARE(active.last(), QStringLiteral("角色：第三答"));
}

void ContextHistoryCompressorTests::buildsFastHistoryWithoutMutatingFullHistory()
{
    using namespace ContextHistoryCompressor;
    const QStringList history{
        QStringLiteral("用户：第一问"), QStringLiteral("角色：第一答"),
        QStringLiteral("用户：第二问"), QStringLiteral("角色：第二答"),
        QStringLiteral("用户：第三问"), QStringLiteral("角色：第三答")};

    const QStringList fast =
        fastActiveHistory(history, QStringLiteral("第一轮摘要"), 2, 4);
    const QStringList expected{
        QStringLiteral("较早对话摘要：第一轮摘要"),
        QStringLiteral("用户：第三问"),
        QStringLiteral("角色：第三答")};
    QCOMPARE(fast, expected);
    QCOMPARE(history.size(), 6);

    const QStringList fastWithoutSummary =
        fastActiveHistory(history, QString(), 0, 2);
    QCOMPARE(fastWithoutSummary, history.mid(2));
    QCOMPARE(fastActiveHistory(history, QString(), 0, 7), history);
}

void ContextHistoryCompressorTests::createsPlanOnConversationBoundary()
{
    using namespace ContextHistoryCompressor;
    const QStringList history{
        QStringLiteral("用户：第一问"), QStringLiteral("角色：第一答"),
        QStringLiteral("用户：第二问"), QStringLiteral("角色：第二答"),
        QStringLiteral("用户：第三问"), QStringLiteral("角色：第三答")};

    const Plan firstPlan = createPlan(history, QString(), 0);
    QVERIFY(firstPlan.isValid());
    QCOMPARE(firstPlan.compactUntil, 2);
    QVERIFY(firstPlan.source.contains(QStringLiteral("第一问")));
    QVERIFY(!firstPlan.source.contains(QStringLiteral("第二问")));
    QCOMPARE(firstPlan.contextToReplace,
             QStringLiteral("用户：第一问\n角色：第一答"));

    const QStringList extended =
        history + QStringList{QStringLiteral("用户：第四问"),
                              QStringLiteral("角色：第四答")};
    const Plan nextPlan =
        createPlan(extended, QStringLiteral("第一轮摘要"), 2);
    QVERIFY(nextPlan.isValid());
    QCOMPARE(nextPlan.compactedHistoryCount, 2);
    QCOMPARE(nextPlan.compactUntil, 4);
    QVERIFY(nextPlan.source.contains(QStringLiteral("第一轮摘要")));
    QVERIFY(nextPlan.source.contains(QStringLiteral("第二问")));
    QVERIFY(nextPlan.contextToReplace.startsWith(
        QStringLiteral("较早对话摘要：第一轮摘要")));
}

void ContextHistoryCompressorTests::skipsPlanWithoutEnoughOldHistory()
{
    using namespace ContextHistoryCompressor;
    const QStringList history{
        QStringLiteral("用户：第一问"), QStringLiteral("角色：第一答"),
        QStringLiteral("用户：第二问"), QStringLiteral("角色：第二答")};

    QVERIFY(!createPlan(history, QString(), 0).isValid());
}

void ContextHistoryCompressorTests::rejectsSummaryThatDoesNotReduceContext()
{
    using namespace ContextHistoryCompressor;
    const QString contextToReplace =
        QStringLiteral("用户：今天去了公园\n角色：听起来很开心");
    QVERIFY(isUsefulSummary(QStringLiteral("去了公园"),
                            contextToReplace));
    QVERIFY(!isUsefulSummary(contextToReplace + contextToReplace,
                             contextToReplace));
    QVERIFY(!isUsefulSummary(QString(), contextToReplace));
}

QTEST_MAIN(ContextHistoryCompressorTests)

#include "test_contexthistorycompressor.moc"
