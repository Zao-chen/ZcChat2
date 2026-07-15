#include "log.h"
#include "createwin.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>
#include <QWidget>

#include <cstdlib>
#include <iostream>

namespace
{
QMutex &logMutex()
{
    // Keep the mutex alive until process exit so late shutdown messages remain safe.
    static QMutex *mutex = new QMutex;
    return *mutex;
}

QString messageTypeName(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:
        return QStringLiteral("Debug");
    case QtInfoMsg:
        return QStringLiteral("Info");
    case QtWarningMsg:
        return QStringLiteral("Warning");
    case QtCriticalMsg:
        return QStringLiteral("Critical");
    case QtFatalMsg:
        return QStringLiteral("Fatal");
    }

    return QStringLiteral("Unknown");
}

QString activeWindowName()
{
    QCoreApplication *application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread())
        return QStringLiteral("UnknownWindow");

    QWidget *window = QApplication::activeWindow();
    return window ? QString::fromLatin1(window->metaObject()->className())
                  : QStringLiteral("UnknownWindow");
}

void customMessageHandler(QtMsgType type, const QMessageLogContext &context,
                          const QString &message)
{
    // User-facing logs keep operational information and above; verbose debug
    // output from this application and linked libraries is intentionally dropped.
    if (type == QtDebugMsg)
        return;

    QString logLine = QStringLiteral("%1 [%2] [%3] %4")
                          .arg(QDateTime::currentDateTime().toString(
                                   QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
                               messageTypeName(type), activeWindowName(), message);

    if (context.category && *context.category
        && QString::fromLatin1(context.category) != QStringLiteral("default"))
    {
        logLine += QStringLiteral(" [%1]").arg(QString::fromLatin1(context.category));
    }

    if (context.file && *context.file)
    {
        logLine += QStringLiteral(" (%1:%2)")
                       .arg(QString::fromLocal8Bit(context.file))
                       .arg(context.line);
    }

    {
        QMutexLocker locker(&logMutex());
        QFile file(QT_LOG::logFilePath());
        if (file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            file.write(logLine.toUtf8());
            file.write("\n");
            file.flush();
        }

        // Keep the legacy ZcChat behavior for terminal launches and redirects.
        std::cout << logLine.toLocal8Bit().constData() << std::endl;
    }

    if (type == QtCriticalMsg)
        createwin(logLine);

    if (type == QtFatalMsg)
        std::abort();
}
} // namespace

namespace QT_LOG
{
QString logFilePath()
{
    QString documentsPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsPath.isEmpty())
        documentsPath = QDir::homePath();

    return QDir(documentsPath).filePath(QStringLiteral("ZcChat2/log.txt"));
}

void logInit()
{
    const QString path = logFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    // Match the previous ZcChat behavior: each launch starts with a fresh log.
    {
        QMutexLocker locker(&logMutex());
        QFile file(path);
        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    }

    qInstallMessageHandler(customMessageHandler);
    qInfo().noquote()
        << "logging.initialized path=" << QDir::toNativeSeparators(path);
}
} // namespace QT_LOG
