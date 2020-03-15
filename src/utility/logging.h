#ifndef _LOGGING_H
#define _LOGGING_H

#include <cstdio>
#include <iostream>
#include <cassert>

#define TRACE(format, ...) printf(format, __VA_ARGS__); std::cout.flush();

#ifdef DEBUG
#    define HEADER(type) printf("%s %s ==> ", type, __PRETTY_FUNCTION__);
#    define LOG(format, ...) TRACE(format, __VA_ARGS__)
#else
#    define HEADER(type) printf("%s ", type);
#    define LOG(format, ...)
#endif

#define NOTE(format, ...) HEADER("n"); TRACE(format, __VA_ARGS__);

#define WARN(format, ...) HEADER("w"); TRACE(format, __VA_ARGS__);

#define ERROR(format, ...) HEADER("e"); TRACE(format, __VA_ARGS__);

#define FATAL(format, ...) HEADER("f"); TRACE(format, __VA_ARGS__); assert(0);

#endif
