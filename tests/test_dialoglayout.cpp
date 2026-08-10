#include "ui_dialog.h"

#include <QApplication>
#include <QWidget>
#include <QtTest>

class DialogLayoutTests : public QObject
{
    Q_OBJECT

  private slots:
    void keepsContextProgressVisibleAtMinimumWidth();
};

namespace
{
QRect geometryInDialog(QWidget *widget, QWidget *dialog)
{
    return QRect(widget->mapTo(dialog, QPoint(0, 0)), widget->size());
}
} // namespace

void DialogLayoutTests::keepsContextProgressVisibleAtMinimumWidth()
{
    QWidget dialog;
    Ui::Dialog ui;
    ui.setupUi(&dialog);
    ui.progressBar_contextTokens->setRange(0, 100);
    ui.progressBar_contextTokens->setValue(72);
    dialog.resize(320, 120);
    dialog.show();
    QApplication::processEvents();

    const QRect nameGeometry = geometryInDialog(ui.label_name, &dialog);
    const QRect hoverGeometry =
        geometryInDialog(ui.widget_contextTokenHoverArea, &dialog);
    const QRect tokenGeometry =
        geometryInDialog(ui.progressBar_contextTokens, &dialog);
    const QRect speechGeometry =
        geometryInDialog(ui.pushButton_input, &dialog);

    QVERIFY(ui.widget_contextTokenHoverArea->isVisible());
    QVERIFY(ui.progressBar_contextTokens->isVisible());
    QCOMPARE(hoverGeometry.size(), QSize(96, 24));
    QCOMPARE(tokenGeometry.size(), QSize(80, 4));
    QVERIFY(hoverGeometry.contains(tokenGeometry));
    QVERIFY(!ui.progressBar_contextTokens->isTextVisible());
    QVERIFY(ui.progressBar_contextTokens->toolTip().contains(
        QStringLiteral("Token")));
    QCOMPARE(ui.widget_contextTokenHoverArea->toolTip(),
             ui.progressBar_contextTokens->toolTip());
    QVERIFY(nameGeometry.right() < hoverGeometry.left());
    QVERIFY(hoverGeometry.right() <= dialog.contentsRect().right());
    QVERIFY(hoverGeometry.bottom() < speechGeometry.top());
}

QTEST_MAIN(DialogLayoutTests)

#include "test_dialoglayout.moc"
