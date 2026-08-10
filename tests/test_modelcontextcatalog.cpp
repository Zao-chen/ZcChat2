#include "ModelContextCatalog.h"

#include <QJsonDocument>
#include <QtTest>

class ModelContextCatalogTests : public QObject
{
    Q_OBJECT

  private slots:
    void matchesStoredSelections();
    void matchesBuiltInProviders();
    void matchesQualifiedAndVariantIds();
    void acceptsOnlyUniqueBareIds();
    void doesNotCrossMatchBuiltInProviders();
    void rejectsInvalidInputs();
};

namespace
{
const QJsonObject Catalog = QJsonDocument::fromJson(R"JSON({
    "openai/gpt-4o": {"limit": {"context": 128000}},
    "deepseek/deepseek-chat": {"limit": {"context": 1000000}},
    "anthropic/claude-sonnet": {"limit": {"context": 200000}},
    "vendor/shared-model": {"limit": {"context": 32000}},
    "other/shared-model": {"limit": {"context": 64000}}
})JSON").object();
} // namespace

void ModelContextCatalogTests::matchesStoredSelections()
{
    QVERIFY(ModelContextCatalog::selectionMatches(
        QString(), QStringLiteral("gpt-4o"), QStringLiteral("OpenAI"),
        QStringLiteral("gpt-4o")));
    QVERIFY(ModelContextCatalog::selectionMatches(
        QStringLiteral("openai"), QStringLiteral("gpt-4o"),
        QStringLiteral("OpenAI"), QStringLiteral("gpt-4o")));
    QVERIFY(!ModelContextCatalog::selectionMatches(
        QStringLiteral("DeepSeek"), QStringLiteral("gpt-4o"),
        QStringLiteral("OpenAI"), QStringLiteral("gpt-4o")));
    QVERIFY(!ModelContextCatalog::selectionMatches(
        QStringLiteral("OpenAI"), QStringLiteral("gpt-4o"),
        QStringLiteral("OpenAI"), QStringLiteral("gpt-4.1")));
}

void ModelContextCatalogTests::matchesBuiltInProviders()
{
    const auto openAi = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("OpenAI"), QStringLiteral("gpt-4o"));
    QCOMPARE(openAi.canonicalModelId, QStringLiteral("openai/gpt-4o"));
    QCOMPARE(openAi.contextTokenLimit, 128000);

    const auto deepSeek = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("DeepSeek"),
        QStringLiteral("deepseek-chat"));
    QCOMPARE(deepSeek.contextTokenLimit, 1000000);
}

void ModelContextCatalogTests::matchesQualifiedAndVariantIds()
{
    const auto qualified = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("Custom"),
        QStringLiteral("anthropic/claude-sonnet"));
    QCOMPARE(qualified.contextTokenLimit, 200000);

    const auto variant = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("Custom"),
        QStringLiteral("anthropic/claude-sonnet:free"));
    QCOMPARE(variant.canonicalModelId,
             QStringLiteral("anthropic/claude-sonnet"));
}

void ModelContextCatalogTests::acceptsOnlyUniqueBareIds()
{
    const auto unique = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("Custom"), QStringLiteral("gpt-4o"));
    QCOMPARE(unique.contextTokenLimit, 128000);

    const auto variant = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("Custom"),
        QStringLiteral("gpt-4o:free"));
    QCOMPARE(variant.canonicalModelId, QStringLiteral("openai/gpt-4o"));

    const auto ambiguous = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("Custom"),
        QStringLiteral("shared-model"));
    QVERIFY(!ambiguous.isValid());
}

void ModelContextCatalogTests::doesNotCrossMatchBuiltInProviders()
{
    const auto wrongProvider = ModelContextCatalog::findContextWindow(
        Catalog, QStringLiteral("OpenAI"),
        QStringLiteral("claude-sonnet"));
    QVERIFY(!wrongProvider.isValid());

    const auto qualifiedWrongProvider =
        ModelContextCatalog::findContextWindow(
            Catalog, QStringLiteral("OpenAI"),
            QStringLiteral("anthropic/claude-sonnet"));
    QVERIFY(!qualifiedWrongProvider.isValid());
}

void ModelContextCatalogTests::rejectsInvalidInputs()
{
    QVERIFY(!ModelContextCatalog::findContextWindow(
                 QJsonObject(), QStringLiteral("OpenAI"),
                 QStringLiteral("gpt-4o"))
                 .isValid());
    QVERIFY(!ModelContextCatalog::findContextWindow(
                 Catalog, QStringLiteral("OpenAI"), QString())
                 .isValid());
}

QTEST_APPLESS_MAIN(ModelContextCatalogTests)

#include "test_modelcontextcatalog.moc"
