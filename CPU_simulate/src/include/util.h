#pragma once
#ifndef __UTIL_H__

#define __UTIL_H__
#include "defs.h"

inline TP now() { return std::chrono::high_resolution_clock::now(); }

int Open(const char* file, int flag, int perm = NEW_FILE_PERM);

void Write(int fd, const void* buf, size_t len);

#endif