#pragma once
#ifndef __LOGGER_H__

#define __LOGGER_H__
#include <stdio.h>

#define LOG_INFO(fmt, args...) ({printf("\033[0m\033[1;34m[INFO] \033[0m"); printf(fmt, ##args); printf("\n"); fflush(stdout);})
#define LOG_ERROR(fmt, args...) ({printf("\033[0m\033[1;31m[ERROR] \033[0m"); printf(fmt, ##args); printf("\n"); fflush(stdout);})
#define LOG_RESULT(fmt, args...) ({printf("\033[0m\033[1;35m[RESULT] \033[0m"); printf(fmt, ##args); printf("\n"); fflush(stdout);})

#ifdef DEBUGMODE
#define LOG_DEBUG(fmt, args...) ({printf("\033[0m\033[1;33m[DEBUG] \033[0m"); printf(fmt, ##args); printf("\n"); fflush(stdout);})
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#endif