#ifndef __LOGGER_H_
#define __LOGGER_H_

#include <stdio.h>

// Uncomment to use printf handler of log
#define LOG_ENABLED 

#ifdef LOG_ENABLED 
#define LOGF(...) printf(__VA_ARGS__)
#else
#define LOGF(...)
#endif

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define __LINE_S__ STRINGIZE(__LINE__)
#define __WHEREAMI__ __FILE__ ":" __LINE_S__

// Log colors definition
#define LOG_CNOM  "\x1B[0m"  // NORMAL
#define LOG_CRED  "\x1B[31m" // RED
#define LOG_CGRN  "\x1B[32m" // GREEN
#define LOC_CYEL  "\x1B[33m" // YELLOW

#define LOG_INFO(X) {vPortEnterCritical(); LOGF("%sINFO:: " X "\n", LOG_CGRN); vPortExitCritical();}
#define LOG_WARN(X) {vPortEnterCritical(); LOGF("%sWARN:: " X "\n", LOC_CYEL); vPortExitCritical();}
#define LOG_ERR(X)  {vPortEnterCritical(); LOGF("%sERR::  " X "\n", LOG_CRED); vPortExitCritical();}

#define LOG_VAR(X) {vPortEnterCritical(); _Generic((X), \
                    default  : LOGF("VAR:: unhandled type\n") , \
                    uint8_t  : LOGF("VAR:: (uint8_t)  %s = %u\n", #X, X ), \
                    uint16_t : LOGF("VAR:: (uint16_t) %s = %u\n", #X, X ), \
                    uint32_t : LOGF("VAR:: (uint32_t) %s = %d\n", #X, X ), \
                    int8_t   : LOGF("VAR:: (int8_t)   %s = %d\n", #X, X ), \
                    int16_t  : LOGF("VAR:: (int16_t)  %s = %d\n", #X, X ), \
                    int32_t  : LOGF("VAR:: (int32_t)  %s = %d\n", #X, X ), \
                    float    : LOGF("VAR:: (float)    %s = %.3f\n", #X, X ), \
                    char     : LOGF("VAR:: (char)     %s = %c\n", #X, X ) ); vPortExitCritical();}

#endif