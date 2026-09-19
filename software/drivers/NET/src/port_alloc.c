#include "bitmap.h"

bitmap_t port_bitmap;
bmp_size_t port_table[NUM_BIT2NUM_UINT(65536)];

#define INVALID_PORT 0

void set_port0_invalid(void)
{
    bitmap_bit_clear(&port_bitmap, INVALID_PORT);
}

/* 在多网卡情况下，多个网卡共用65536个端口号。端口号基于应用，是全局的*/
void net_alloc_port_init(void)
{
    bitmap_create_full(&port_bitmap, port_table, 65536);
}

net_err_t net_alloc_port(uint16_t * port)
{
    * port = bitmap_lowest_set_idx(&port_bitmap);
    if( * port == INVALID_PORT )
        return NET_ERR_NOK;
    bitmap_bit_clear(&port_bitmap, * port);
    return NET_ERR_OK;
}

void net_free_port(uint16_t port)
{
    bitmap_bit_set(&port_bitmap, port);
}