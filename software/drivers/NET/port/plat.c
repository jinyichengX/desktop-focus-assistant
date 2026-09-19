#include "plat.h"
#include "nmem.h"
#if defined(ELNET_PLAT_WINDOWS)
/* sem operations */
sys_sem_t sys_sem_create(uint16_t val)
{
    return CreateSemaphore(NULL, val, 0xffff, NULL);
}

ret_t sys_sem_destroy(sys_sem_t sem)
{
    return (ret_t)CloseHandle(sem);
}

ret_t sys_sem_take(sys_sem_t sem, uint32_t timeout_tick)
{
    if (sem == NULL)
        return -1;
    DWORD result = WaitForSingleObject(sem, timeout_tick);
    if (result == WAIT_OBJECT_0)
        return 0;
    else if (result == WAIT_TIMEOUT)
        return -2;
    else
        return -1;
}

void sys_sem_release(sys_sem_t sem)
{
    if( sem == NULL ){
        plat_printf("param error");
        return;
    }
	  if(ReleaseSemaphore(sem, 1, NULL) == 0){
        plat_printf("release error\r\n");
    }
}

/* mutex operations */
sys_mutex_t sys_mutex_create(void)
{
    return CreateMutex(NULL, FALSE, NULL);
}

ret_t sys_mutex_destroy(sys_mutex_t mutex)
{
    return 0;
}

ret_t sys_mutex_lock(sys_mutex_t mutex)
{
    if (mutex == NULL)
        return -1;

    DWORD result = WaitForSingleObject(mutex, INFINITE);
    if (result == WAIT_OBJECT_0)
        return 0;
    else
        return -1;
}

void sys_mutex_unlock(sys_mutex_t mutex)
{
    if (mutex != NULL && !ReleaseMutex(mutex))
    {
        plat_printf("param error");
    }
}

/*thread operations*/
sys_thread_t sys_thread_create(thread_code_t thread_code, void * args)
{
    sys_thread_t handle = NULL;
    handle = CreateThread(NULL, 0, thread_code, args, 0, NULL);
    if(handle == 0){
        plat_printf("error: create thread error!\r\n");
    }
    return handle;
}

void sys_thread_sleep(size_t ms)
{
    Sleep(ms);
}

sys_tick_t sys_tick_now(void)
{
    return GetTickCount();
}

sys_tick_t sys_tick_gone(sys_tick_t last)
{
    return sys_tick_now() - last;
}

pcap_t * sys_netif_Initialise(void)
{
	char* dev = NULL;
	pcap_if_t* alldevs, * d;
	int ret;
	struct pcap_pkthdr header;
	const u_char* packet;
	int i = 0;
	char errbuf[PCAP_ERRBUF_SIZE];

	/* Retrieve the device list from the local machine */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL /* auth is not needed */, &alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
		exit(1);
	}

	/* Print the list */
	for (d = alldevs; d != NULL; d = d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
		{
			printf(" (%s)\n", d->description);
		}
		else
			printf(" (No description available)\n");

		if (strstr(d->description, "Realtek PCIe GbE Family Controller")) {
			dev = (char*)d->name;
		}
	}

	pcap_t* adhandle = pcap_open(dev, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, NULL, errbuf);
	if (adhandle == NULL) {
		fprintf(stderr, "Error in pcap_open: %s\n", errbuf);
		return NULL;
	}

	return adhandle;
}


#elif defined(ELNET_PLAT_LINUX)

sys_thread_t sys_thread_create(thread_code_t thread_code, void * args)
{
    /* not return 0 if failed */
    pthread_t * thread = NULL;

    thread = (pthread_t *)malloc(sizeof(pthread_t));

    if(0 != pthread_create(thread, NULL, thread_code, args)){
        free(thread);
        return SYS_THREAD_INVALID;
    }
    return thread;
}

sys_sem_t sys_sem_create(void)
{

}

/* 没有做系统时基和时间的转换，默认1tick = 1ms */
/* 此接口有问题 */
sys_sem_t sys_sem_take(sys_sem_t sem, uint32_t timeout_tick)
{
    struct timeval now;
    struct timespec outtime;

    pthread_mutex_unlock(sem->mutex);

    gettimeofday(&now, NULL);

    outtime.tv_sec = now.tv_sec + timeout_tick / 1000;
    outtime.tv_nsec = now.tv_usec * 1000 + (timeout_tick % 1000) * 1000000;

    pthread_cond_timedwait(sem->cond, sem->mutex, &outtime);
    
    pthread_mutex_lock(sem->mutex);
}

sys_mutex_t sys_mutex_create(void)
{
    pthread_mutex_t * mutex = NULL;

    mutex = malloc(sizeof(pthread_mutex_t));
    if( !mutex ){
        return NULL;
    }
    if(pthread_mutex_init(NULL) != 0){
        free(mutex);
        return NULL;
    }

    return mutex;
}

ret_t sys_mutex_lock(sys_mutex_t mutex)
{
    return pthread_mutex_lock(mutex);
}

void sys_mutex_unlock(sys_mutex_t mutex)
{
    pthread_mutex_unlock(mutex);
}

#elif defined(ELNET_PLAT_ELOS)
EL_G_SYSTICK_TYPE systick_get(void);
/* sem operations */
sys_sem_t sys_sem_create(uint16_t val)
{
	return el_sem_create((uint32_t)val);
}

ret_t sys_sem_take(sys_sem_t sem, uint32_t timeout_tick)
{
	return el_sem_take(sem, (uint32_t)timeout_tick);
}

void sys_sem_release(sys_sem_t sem)
{
	  el_sem_release(sem);
}

ret_t sys_sem_destroy(sys_sem_t sem)
{
    return el_sem_destroy(sem);
}

/*mutex operations*/
sys_mutex_t sys_mutex_create(void)
{
	return mutex_create(MUTEX_LOCK_NESTING);
}

ret_t sys_mutex_destroy(sys_mutex_t mutex)
{
	return mutex_destroy(mutex);
}

ret_t sys_mutex_lock(sys_mutex_t mutex)
{
	return EL_MutexLock_Take(mutex);
}

void sys_mutex_unlock(sys_mutex_t mutex)
{
	EL_MutexLock_Release(mutex);
}

/*thread operations*/
sys_thread_t sys_thread_create(thread_code_t thread_code, void * args)
{
    sys_thread_t handle = NULL;
    handle = thread_create(NULL, thread_code, 1024 , 0, args);
    if(handle == 0){
        plat_printf("warning: create thread error!\r\n");
    }
    return handle;
}

void sys_thread_sleep(size_t ms)
{
	thread_sleep((EL_UINT)ms);
}

sys_tick_t sys_tick_now(void)
{
    return systick_get();
}

sys_tick_t sys_tick_gone(sys_tick_t last)
{
    return sys_tick_now() - last;
}
#elif defined(ELNET_PLAT_UCOS)

/* sem operations */
sys_sem_t sys_sem_create(uint16_t val)
{
	return OSSemCreate((uint32_t)val);
}

ret_t sys_sem_take(sys_sem_t sem, uint32_t timeout_tick)
{
	INT8U ret = 0;
	if (timeout_tick == 0xffffffff)
		OSSemPend(sem, (uint32_t)0, &ret);
	else 
		OSSemPend(sem, (uint32_t)timeout_tick, &ret);
	if (ret == OS_ERR_NONE) return 0;
	else return 1;
}

void sys_sem_release(sys_sem_t sem)
{
	  OSSemPost(sem);
}

ret_t sys_sem_destroy(sys_sem_t sem)
{
	INT8U ret;
    if((OS_EVENT *)0 == OSSemDel(sem, 0, &ret))
		return 0;
	else return 1;
}

/*mutex operations*/
sys_mutex_t sys_mutex_create(void)
{
		return OSSemCreate(1);
//	INT8U ret;
//	sys_mutex_t p = (sys_mutex_t)0;
//	static int used_prio = 10;
//	p = OSMutexCreate(used_prio, &ret);
//	if (p){
//		used_prio ++;
//	}
//	return p;
}

ret_t sys_mutex_destroy(sys_mutex_t mutex)
{
	sys_sem_destroy(mutex);
//	return mutex_destroy(mutex);
}

ret_t sys_mutex_lock(sys_mutex_t mutex)
{
	sys_sem_take(mutex, 0xffffffff);
//	INT8U ret = 0;
//	OSMutexPend(mutex, 0xffffffff, &ret);
//	if (ret == OS_ERR_NONE) return 0;
//	else return 1;
}

void sys_mutex_unlock(sys_mutex_t mutex)
{
	sys_sem_release(mutex);
//	OSMutexPost(mutex);
}


extern mem_mng_t g_mem_mng;
/*thread operations*/
sys_thread_t sys_thread_create(thread_code_t thread_code, void * args)
{
	static int used_prio = 30;
    sys_thread_t handle = NULL;
	void * p = mem_alloc(&g_mem_mng, 2048);
	if (!p) return 12;
    handle = OSTaskCreateExt(thread_code, 
							args, 
							(OS_STK *)((char *)p + 2047),
							used_prio, 
							used_prio,
							(OS_STK *)p,
							1024,
							(void *)0,
							(INT16U )0);
    if(handle != 0){
//        plat_printf("warning: create thread error!\r\n");
    } else used_prio ++;
    return handle;
}

void sys_thread_sleep(size_t ms)
{
//	thread_sleep((EL_UINT)ms);
}

sys_tick_t sys_tick_now(void)
{
    return OSTimeGet();
}

sys_tick_t sys_tick_gone(sys_tick_t last)
{
    return sys_tick_now() - last;
}
#endif
