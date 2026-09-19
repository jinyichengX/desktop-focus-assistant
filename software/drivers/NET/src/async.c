/*
 * MIT License
 *
 * Copyright (c) 2024~2025 JinYiCheng
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "async.h"

#define ASYNC_REQUEST_NUM 2
mempool_t async_req_pool;
async_req_t async_req[ASYNC_REQUEST_NUM];

/* async handler thread */
int async_handler_thread(void * args)
{
    async_req_t * pstAsyncHandleItem = NULL;
#define ASYNC_QUEUE_FETCH(pItem) queue_fetch_ptr(&async_handler_queue, (pItem), WAIT_FOREVER)
    while(1)
    {
        /* fetch async request */
        ASYNC_QUEUE_FETCH((void *)&pstAsyncHandleItem);
        if( NULL == pstAsyncHandleItem )
            continue;

        /* handle async request */
        if( pstAsyncHandleItem->pbResult )
            * (pstAsyncHandleItem->pbResult) = pstAsyncHandleItem->pfCallBack(pstAsyncHandleItem->pArgs);
        else
            pstAsyncHandleItem->pfCallBack(pstAsyncHandleItem->pArgs);
        if( WAIT_RET == pstAsyncHandleItem->eWaitRet)
        {
            /* give sem */
            sys_sem_release(pstAsyncHandleItem->pSem);
        }

        pstAsyncHandleItem = NULL;
        mempool_free(&async_req_pool, (void *)pstAsyncHandleItem);
    }

    return 0;
}

/* async request module init */
char async_init(void)
{
#define ASYNC_QUEUE_POST(pItem)  queue_post_ptr(&async_handler_queue, (pItem), WAIT_FOREVER);
    async_thread_handler = sys_thread_create(async_handler_thread, NULL);
    if( NULL == async_handler_thread )
        return -1;

    queue_init(&async_handler_queue, async_handler_pool, ASYNC_MQ_ITEM_NUM, sizeof(void *));
    mempool_create(&async_req_pool, async_req, sizeof(async_req_t), ASYNC_REQUEST_NUM);

    return 0;
}

/* async request */
int async_once_request_post(async_callback_t pfCallBack, void * pParam, wait_ret_t eWaitRet, int * pbResult)
{
    /* 这里最好不要使用栈变量？ */
    ret_t unSemTakeRet;

    async_req_t * pstRequest = (async_req_t *)mempool_alloc(&async_req_pool, WAIT_FOREVER);

    pstRequest->pSem = NULL;
    if( eWaitRet != NWAIT_RET )
    {
        if(NULL == (pstRequest->pSem = sys_sem_create(0)))
        {
            /* handle error */
            return (int)NET_ERR_NOK;
        }
    }

    pstRequest->pfCallBack = pfCallBack;
    pstRequest->eWaitRet = eWaitRet;
    pstRequest->pbResult = pbResult;
    pstRequest->pArgs = pParam;

    ASYNC_QUEUE_POST((void *)pstRequest);

    if( eWaitRet == WAIT_RET )
    {
        /* wait for async handle finished */
        unSemTakeRet = sys_sem_take( pstRequest->pSem, 0xffffffff );
        sys_sem_release( pstRequest->pSem );
        sys_sem_destroy( pstRequest->pSem );

        return *(pstRequest->pbResult);
    }

    return (int)NET_ERR_OK;
}

int async_period_request_post(async_callback_t pfCallBack,void * pParam,wait_ret_t eWaitRet,int * pbResult,eIsPeriodic_t eIsPeriodic)
{
    async_req_t stRequest;

    stRequest.pfCallBack = pfCallBack;
    stRequest.eWaitRet = eWaitRet;
    stRequest.pbResult = pbResult;
    stRequest.pArgs = pParam;

    return (int)NET_ERR_OK;
}

void async_request_cancel(async_req_t * pstRequest)
{

}
//更多的功能：1. 使用优先级队列来处理异步请求 2.请求超时处理3.请求取消功能4.多线程扩展支持多个异步处理线程（线程池？）5.状态反馈：
//允许请求者查询异步请求的状态，例如是否正在处理、已完成或已取消6.延时处理异步请求

//3.请求取消功能场景如下
//用户操作：用户可能在长时间运行的异步操作完成前改变了意图，例如，用户可能在文件下载过程中取消下载。
//依赖变更：如果异步操作依赖于某些条件或数据，而这些条件或数据在操作进行中发生了变化，可能需要取消操作。
//错误预防：如果检测到系统资源不足或其他潜在的错误风险，可能需要取消正在进行的请求以避免系统崩溃或数据损坏。
//超时处理：对于预计在特定时间内完成的操作，如果超出了预期时间，可能需要取消以释放资源。
//资源清理：在应用程序关闭或重新启动前，可能需要取消所有未完成的异步操作，以确保资源被正确清理。
//优先级调整：在多任务环境中，可能需要取消低优先级的请求以释放资源，以便高优先级的请求可以使用这些资源。
