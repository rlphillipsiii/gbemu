#ifndef _LOGGING_H
#define _LOGGING_H

#ifdef DEBUG
#include <cstdio>
#include <iostream>
#define LOG(format, ...) printf(format, __VA_ARGS__); std::cout.flush();
#else
#define LOG(format, ...)
#endif

#endif
