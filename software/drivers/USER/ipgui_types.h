#ifndef IPGUI_TYPES_H
#define IPGUI_TYPES_H

#include "ipgui_utils.h"

typedef enum
{
    IPGUI_ERR_OK                = 0,
    IPGUI_ERR_NOK               = -1,

    IPGUI_ERR_PARAM             = 1,/* 传参错误 */
    IPGUI_ERR_OVERFLOW          = 2,             /* parameter result in overflow */
    IPGUI_ERR_OBJ_INVALID       = 3,

    IPGUI_ERR_MEM               = 5,
    IPGUI_ERR_NOMEM             = 6,

    IPGUI_ERR_EVT_NOT_SUPPORTED = 7,

    IPGUI_ERR_FS_FWRITE         = 8,
    IPGUI_ERR_FS_FREAD          = 9,
    IPGUI_ERR_FS_FOPEN          = 10,
    IPGUI_ERR_FS_FCLOSE         = 11,
    IPGUI_ERR_FS_MISC           = 12,
    IPGUI_ERR_FS_DENTER         = 13,
    IPGUI_ERR_FS_DRENAME        = 15,
    IPGUI_ERR_FS_DCREATE        = 16,

    IPGUI_ERR_QUEUE_FULL        = 17,
    IPGUI_ERR_QUEUE_EMPTY       = 18,
    IPGUI_ERR_LOGIC             = 19,
    IPGUI_ERR_CHILDREN_LIMIT_EXCEEDED = 19
}ipgui_err_t;

#if defined(IPGUI_BASETYPE_64BIT)
typedef unsigned int ipgui_tick_t;
#else
typedef unsigned int ipgui_tick_t;
#endif

#endif

