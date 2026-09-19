#ifndef LIB_H
#define LIB_H

/* macro begin and end */
#define __MACRO_BEGIN       do {
#define __MACRO_END         } while (0)

/* do while loop */
#define __LOOP_BEGIN        __MACRO_BEGIN
#define __LOOP_END(cond)    } while ((cond))

#endif