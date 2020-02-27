#ifndef _LOGGING_H
#define _LOGGING_H

#include <cstdio>
#include <iostream>

#define TRACE(format, ...) printf(format, __VA_ARGS__); std::cout.flush();

#ifdef DEBUG

#define LOG(format, ...) TRACE(format, __VA_ARGS__)
#else
#define LOG(format, ...)
#endif

#endif

#define NOTE(format, ...) printf("n %s ==> ", __PRETTY_FUNCTION__); TRACE(format, __VA_ARGS__);

#define WARN(format, ...) printf("w %s ==> ", __PRETTY_FUNCTION__); TRACE(format, __VA_ARGS__);

#define ERROR(format, ...) printf("e %s ==> ", __PRETTY_FUNCTION__); TRACE(format, __VA_ARGS__);
