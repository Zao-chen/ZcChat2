#ifndef ZCAILIB_GLOBAL_H
#define ZCAILIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(ZCAILIB_LIBRARY)
#  define ZCAILIB_EXPORT Q_DECL_EXPORT
#else
#  define ZCAILIB_EXPORT Q_DECL_IMPORT
#endif

#endif // ZCAILIB_GLOBAL_H