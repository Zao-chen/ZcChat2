#ifndef ZCJSONLIB_GLOBAL_H
#define ZCJSONLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(ZCJSONLIB_LIBRARY)
#define ZCJSONLIB_EXPORT Q_DECL_EXPORT
#else
#define ZCJSONLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // ZCJSONLIB_GLOBAL_H
