#ifndef ASYNC_H
#define ASYNC_H

#include "plat.h"
#include "queue.h"
#include "global.h"

typedef int (*async_callback_t)(void * args);

typedef enum
{
    WAIT_RET,
    NWAIT_RET,
}wait_ret_t;

typedef enum
{
    ASYNC_REQ_PERIODIC = 0,
    ASYNC_REQ_ONCE,
}eIsPeriodic_t;

typedef struct _AsyncRequestStruct
{
    async_callback_t pfCallBack;
    sys_sem_t pSem;
    void * pArgs;
    wait_ret_t eWaitRet;
    int * pbResult;
    eIsPeriodic_t eIsPeriodic;
}async_req_t;

extern char async_init(void);
extern int  async_once_request_post(async_callback_t pfCallBack, void * pParam, wait_ret_t eWaitRet, int * pbResult);
extern int  async_period_request_post(async_callback_t pfCallBack, void * pParam, wait_ret_t eWaitRet, int * pbResult, eIsPeriodic_t eIsPeriodic);
#endif