#include "global.h"

#define STATISTIC_ON 1

timer_t nbuf_statistic_timer;

#if NBUF_USAGE_STATISTICS == 1 && STATISTIC_ON == 1
static void statistics_timer_proc(timer_t * tmr, void * args)
{
    uint8_t uu1, uu2, uu3, uu4;
    nbuf_get_statistics(&uu1, &uu2, &uu3, &uu4);
}
#endif

void statistic_init(void)
{
#if STATISTIC_ON == 1
#if NBUF_USAGE_STATISTICS == 1
    net_timer_add(&nbuf_statistic_timer, statistics_timer_proc, RELOAD_FOREVER, 1000, NULL);
#endif
#endif
}

