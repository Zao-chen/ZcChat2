#include "ContextTokenEstimator.h"

#include <QLocale>
#include <QtTest>

class ContextTokenEstimatorTests : public QObject
{
    Q_OBJECT

  private slots:
    void estimatesCommonTextTypes();
    void includesChatMessageOverhead();
    void validatesContextTokenLimits();
    void doesNotReuseLimitsAcrossSelections();
    void clampsRemainingTokensAtZero();
    void formatsProgressToolTip();
};

void ContextTokenEstimatorTests::estimatesCommonTextTypes()
{
    QCOMPARE(ContextTokenEstimator::estimateTextTokens(QString()), 0);
    QCOMPARE(ContextTokenEstimator::estimateTextTokens(
                 QStringLiteral("abcdefghijklmnop")),
             4);
    QCOMPARE(ContextTokenEstimator::estimateTextTokens(
                 QStringLiteral("你好世界")),
             4);
    QCOMPARE(ContextTokenEstimator::estimateTextTokens(
                 QString::fromUtf8("😀")),
             2);
}

void ContextTokenEstimatorTests::includesChatMessageOverhead()
{
    const QString systemPrompt = QStringLiteral("你是桌宠 AI");
    const QString userMessage = QStringLiteral("你好");
    const int textTokens =
        ContextTokenEstimator::estimateTextTokens(systemPrompt) +
        ContextTokenEstimator::estimateTextTokens(userMessage);

    QVERIFY(ContextTokenEstimator::estimateChatRequestTokens(systemPrompt,
                                                             userMessage) >
            textTokens);
}

void ContextTokenEstimatorTests::validatesContextTokenLimits()
{
    QCOMPARE(ContextTokenEstimator::validatedTokenLimit(128000), 128000);
    QCOMPARE(ContextTokenEstimator::validatedTokenLimit(0),
             ContextTokenEstimator::DefaultContextTokenLimit);
    QCOMPARE(ContextTokenEstimator::validatedTokenLimit(
                 ContextTokenEstimator::MaximumContextTokenLimit + 1),
             ContextTokenEstimator::DefaultContextTokenLimit);
}

void ContextTokenEstimatorTests::doesNotReuseLimitsAcrossSelections()
{
    QCOMPARE(ContextTokenEstimator::tokenLimitForSelection(128000, true),
             128000);
    QCOMPARE(ContextTokenEstimator::tokenLimitForSelection(1000000, false),
             ContextTokenEstimator::DefaultContextTokenLimit);
}

void ContextTokenEstimatorTests::clampsRemainingTokensAtZero()
{
    QCOMPARE(ContextTokenEstimator::remainingTokens(100, 40), 60);
    QCOMPARE(ContextTokenEstimator::remainingTokens(100, 120), 0);
}

void ContextTokenEstimatorTests::formatsProgressToolTip()
{
    const QLocale locale = QLocale::c();
    const QString normal =
        ContextTokenEstimator::formatContextProgressToolTip(
            100, 40, QStringLiteral("Models.dev"), locale);
    QVERIFY(normal.contains(QStringLiteral("本地估算剩余：60 Token")));
    QVERIFY(normal.contains(QStringLiteral("本地估算已用：40 Token")));
    QVERIFY(normal.contains(QStringLiteral("上下文上限：100 Token（Models.dev）")));
    QVERIFY(!normal.contains(QStringLiteral("已超出")));

    const QString overflow =
        ContextTokenEstimator::formatContextProgressToolTip(
            100, 120, QStringLiteral("手动设置"), locale);
    QVERIFY(overflow.contains(QStringLiteral("本地估算剩余：0 Token")));
    QVERIFY(overflow.contains(QStringLiteral("已超出：20 Token")));
}

QTEST_APPLESS_MAIN(ContextTokenEstimatorTests)

#include "test_contexttokenestimator.moc"
