#ifndef _LOGGING_H
#define _LOGGING_H

#ifdef DEBUG
#define LOG(format, ...) printf(format, __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#endif
