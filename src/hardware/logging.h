#ifndef _LOGGING_H
#define _LOGGING_H

#include <cstdio>
#include <iostream>

#ifdef DEBUG

#define LOG(format, ...) printf(format, __VA_ARGS__); std::cout.flush();
#else
#define LOG(format, ...)
#endif

#endif

#define TRACE(format, ...) printf(format, __VA_ARGS__); std::cout.flush();
