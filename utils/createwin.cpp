#include "createwin.h"

#include "ElaContentDialog.h"
#include "ElaText.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QMetaObject>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

void createwin(const QString &message)
{
    QCoreApplication *application = QCoreApplication::instance();
    if (!application)
        return;

    const auto showMessage = [message]()
    {
        // Avoid recursively opening another dialog if Ela/Qt reports a critical
        // message while this dialog is being created or displayed.
        static bool isShowing = false;
        if (isShowing)
            return;

        isShowing = true;

        ElaContentDialog dialog(QApplication::activeWindow());
        dialog.setWindowTitle(QStringLiteral("ZcChat2 错误"));
        dialog.setLeftButtonText({});
        dialog.setMiddleButtonText({});
        dialog.setRightButtonText(QStringLiteral("确定"));

        auto *content = new QWidget(&dialog);
        content->setMinimumWidth(480);

        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);

        auto *titleLabel =
            new ElaText(QStringLiteral("发生严重错误"), 18, content);
        auto *messageLabel = new ElaText(message, content);
        messageLabel->setWordWrap(true);
        messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        layout->addWidget(titleLabel);
        layout->addWidget(messageLabel);
        dialog.setCentralWidget(content);

        QObject::connect(&dialog, &ElaContentDialog::rightButtonClicked,
                         &dialog, &QDialog::accept);
        dialog.exec();

        isShowing = false;
    };

    if (QThread::currentThread() == application->thread())
    {
        showMessage();
        return;
    }

    // Qt messages can originate from network or audio worker threads.
    QMetaObject::invokeMethod(application, showMessage, Qt::QueuedConnection);
}
