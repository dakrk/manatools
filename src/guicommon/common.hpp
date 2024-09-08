#pragma once
#include <QtGlobal>

#ifdef GUICOMMON_LIBRARY
	#define GUICOMMON_EXPORT Q_DECL_EXPORT
#else
	#define GUICOMMON_EXPORT Q_DECL_IMPORT
#endif
