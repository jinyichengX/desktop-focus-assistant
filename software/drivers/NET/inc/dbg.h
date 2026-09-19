#ifndef DBG_H
#define DBG_H

#define DBG_INFO_LEVEL  1
#define DBG_WARN_LEVEL  2
#define DBG_ERROR_LEVEL 3

// #include "global.h"

// #define DBG(fmt, ...) \
//     do { \
//         if (DEBUG) \
//             printf(fmt, ##__VA_ARGS__); \
//     } while (0)

#define DBG_ASSERT(cond, message) do { \
                                    if (!(cond)) { \
                                        printk("Assertion failed: %s\n", message); \
                                        while(1); \
                                    } \
                                  } while(0)

#define DBG_INFO(lv, fmt, ...)

#endif