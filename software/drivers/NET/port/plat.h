#ifndef PLAT_H
#define PLAT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* GNUC */
#if defined __GNUC__
#define ST_ALIGN(n) __attribute__((aligned(n)))
#elif defined __clang__
/* standard C */
#if defined __STDC_VERSION__ && (__STDC_VERSION__ >= 201112L)
#define ST_ALIGN(n) _Alignas(n)
#elif
#define ST_ALIGN(n) _Pragma("pack(push, 1)")
#else
#error "Unsupported C/C++ standard"
#endif
#else
#error "Unsupported C/C++ compiler"
#endif

/* OS platform */
#if defined(_WIN32) || defined(_WIN64)
#include "pcap.h"
#include <windows.h>
#include <process.h>

#define ELNET_PLAT_WINDOWS

#define ENDIANNESS_LITTLE 1 //小端为1，大端为0

#define plat_strlen         strlen
#define plat_strcpy         strcpy
#define plat_strncpy        strncpy
#define plat_strcmp         strcmp
#define plat_stricmp        _stricmp
#define plat_memset         memset
#define plat_memcpy         memcpy
#define plat_memcmp         memcmp
#define plat_sprintf        sprintf
#define plat_vsprintf       vsprintf
#define plat_printf         printf

#define SYS_THREAD_INVALID          (HANDLE)0
#define SYS_SEM_INVALID             (HANDLE)0
#define SYS_MUTEX_INVALID           (HANDLE)0

typedef HANDLE sys_mutex_t;         // 互斥锁句柄
typedef HANDLE sys_thread_t;        // 线程句柄
typedef HANDLE sys_sem_t;           // 信号量句柄

typedef DWORD sys_tick_t;           

typedef DWORD (WINAPI *thread_code_t) (LPVOID lpThreadParameter);
#define WAIT_FOREVER 0xFFFFFFFF
typedef int ret_t;

extern pcap_t * sys_netif_Initialise(void);
#elif defined(__linux__)
#include <pthread.h>
#include <sys/time.h>
#define ENDIANNESS_LITTLE 1//小端为1，大端为0

#define ELNET_PLAT_LINUX

#define plat_strlen         strlen
#define plat_strcpy         strcpy
#define plat_strncpy        strncpy
#define plat_strcmp         strcmp
#define plat_stricmp        _stricmp
#define plat_memset         memset
#define plat_memcpy         memcpy
#define plat_memcmp         memcmp
#define plat_sprintf        sprintf
#define plat_vsprintf       vsprintf
#define plat_printf         printf

typedef struct{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
}*                              sys_sem_t;              //信号量句柄
typedef pthread_mutex_t *       sys_mutex_t;            // 互斥锁句柄
typedef pthread_t *             sys_thread_t;           //线程句柄

#define SYS_THREAD_INVALID          (pthread_t *)0
#define SYS_SEM_INVALID             (sys_sem_t)0
#define SYS_MUTEX_INVALID           (pthread_mutex_t *)0

typedef void *(* thread_code_t)(void * args);
#define WAIT_FOREVER 0xFFFFFFFF
typedef EL_RESULT_T ret_t;
#elif defined(__ELOS__)
#include "el_pthread.h"
#include "el_mutex.h"
#include "el_sem.h"
#include "el_type.h"

#define ENDIANNESS_LITTLE 1//小端为1，大端为0

#define ELNET_PLAT_ELOS

#define plat_strlen         strlen
#define plat_strcpy         strcpy
#define plat_strncpy        strncpy
#define plat_strcmp         strcmp
#define plat_stricmp        _stricmp
#define plat_memset         memset
#define plat_memcpy         memcpy
#define plat_memcmp         memcmp
#define plat_sprintf        sprintf
#define plat_vsprintf       vsprintf
#define plat_printf         printf

#define SYS_THREAD_INVALID          (void *)0
#define SYS_SEM_INVALID             (void *)0
#define SYS_MUTEX_INVALID           (void *)0

typedef sem_t *             sys_sem_t;              //信号量句柄
typedef thread_t *          sys_thread_t;           //线程句柄
typedef mutex_lock_t *      sys_mutex_t;            //互斥锁句柄

typedef uint32_t sys_tick_t;

typedef void (* thread_code_t)(void * args);
#define WAIT_FOREVER 0xFFFFFFFF
typedef EL_RESULT_T ret_t;
#elif defined(UCOS)
#include "ucos_ii.h"
#include "os_cpu.h"
#include "os_cfg.h"

#define ENDIANNESS_LITTLE 1//小端为1，大端为0

#define ELNET_PLAT_UCOS

#define plat_strlen         strlen
#define plat_strcpy         strcpy
#define plat_strncpy        strncpy
#define plat_strcmp         strcmp
#define plat_stricmp        _stricmp
#define plat_memset         memset
#define plat_memcpy         memcpy
#define plat_memcmp         memcmp
#define plat_sprintf        sprintf
#define plat_vsprintf       vsprintf
#define plat_printf(...)

#define SYS_THREAD_VALID         0
#define SYS_SEM_INVALID             (void *)0
#define SYS_MUTEX_INVALID           (void *)0

typedef OS_EVENT  *             sys_sem_t;              //信号量句柄
typedef INT8U          sys_thread_t;           //线程句柄
typedef OS_EVENT  *      sys_mutex_t;            //互斥锁句柄

typedef INT32U sys_tick_t;

typedef void (* thread_code_t)(void * args);
#define WAIT_FOREVER 0xFFFFFFFF
typedef int ret_t;
//#error "error: unkown os platform!"
#endif

#if defined(__linux__) || defined(__linux) || defined(linux)
    #include <linux/irqflags.h>  // 包含中断开关相关的头文件

    #define DISABLE_INTERRUPTS() local_irq_disable()
    #define ENABLE_INTERRUPTS() local_irq_enable()
    #define SAVE_AND_DISABLE_INTERRUPTS(flags) local_irq_save(flags)
    #define RESTORE_INTERRUPTS(flags) local_irq_restore(flags)

#elif defined(__FreeRTOS__)
    #include "FreeRTOS.h"
    #include "task.h"

    #define DISABLE_INTERRUPTS() portENTER_CRITICAL()
    #define ENABLE_INTERRUPTS() portEXIT_CRITICAL()
    #define SAVE_AND_DISABLE_INTERRUPTS(flags) (flags = portSET_INTERRUPT_MASK_FROM_ISR())
    #define RESTORE_INTERRUPTS(flags) portCLEAR_INTERRUPT_MASK_FROM_ISR(flags)

#elif defined(RT_THREAD)
    #include <rthw.h>
    #include <rtthread.h>

    #define DISABLE_INTERRUPTS() rt_hw_interrupt_disable()
    #define ENABLE_INTERRUPTS() rt_hw_interrupt_enable(0)
    #define SAVE_AND_DISABLE_INTERRUPTS(flags) (flags = rt_hw_interrupt_disable())
    #define RESTORE_INTERRUPTS(flags) rt_hw_interrupt_enable(flags)

#elif defined(_WIN32) || defined(_WIN64)
    #include <windows.h>

    #define DISABLE_INTERRUPTS() DisableThreadLibraryCalls(GetModuleHandle(NULL))
    #define ENABLE_INTERRUPTS() // Windows 用户态通常不需要手动恢复中断
    #define SAVE_AND_DISABLE_INTERRUPTS(flags) // Windows 用户态不支持此操作
    #define RESTORE_INTERRUPTS(flags) // Windows 用户态不支持此操作

#elif defined(__ELOS__) || defined(__elos__)
    #include "port.h"

    #define DISABLE_INTERRUPTS() OS_Exit_Critical_Check()
    #define ENABLE_INTERRUPTS() OS_Enter_Critical_Check()
    #define SAVE_AND_DISABLE_INTERRUPTS(flags) OS_Enter_Critical_Check()
    #define RESTORE_INTERRUPTS(flags) OS_Exit_Critical_Check()
    
#elif defined(UCOS)
//    #error "Unsupported platform for interrupt control"
#endif

/*sem operations*/
extern sys_sem_t sys_sem_create(uint16_t val);
extern ret_t sys_sem_take(sys_sem_t sem, uint32_t timeout_tick);
extern void sys_sem_release(sys_sem_t sem);
extern ret_t sys_sem_destroy(sys_sem_t sem);

/*mutex operations*/
extern sys_mutex_t sys_mutex_create(void);
extern ret_t sys_mutex_lock(sys_mutex_t mutex);
extern void sys_mutex_unlock(sys_mutex_t mutex);
extern ret_t sys_mutex_destroy(sys_mutex_t mutex);

/*thread operations*/
extern sys_thread_t sys_thread_create(thread_code_t thread_code, void * args);
extern void sys_thread_sleep(size_t ms);

/* get system tick */
extern sys_tick_t sys_tick_now(void);
extern sys_tick_t sys_tick_gone(sys_tick_t last);

#ifdef __cplusplus
}
#endif

#endif // !PLAT_H
