#pragma once

#include <QString>

namespace QT_LOG
{
// Installs a process-wide Qt message handler and starts a fresh log file.
void logInit();

// Returns the path used by the message handler.
QString logFilePath();
} // namespace QT_LOG
