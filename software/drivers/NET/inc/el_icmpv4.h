#ifndef EL_ICMPV4_H
#define EL_ICMPV4_H

#include "elnet.h"
#include "el_netif.h"
#include "el_ndefs.h"
#include "el_nbuf.h"
#include "el_ipv4.h"

#pragma pack(1)
typedef struct icmpv4_hdr_st
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union{
        uint32_t _mzero;
        uint16_t id;
        uint16_t seq;
    };
}icmpv4_hdr_t;
#pragma pack()

#pragma pack(1)
/* ICMPv4 request(code 8)/reply(code 0) */
typedef struct icmpv4_echo_header_st
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
    uint8_t data[];
}icmp_echo_hdr_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t zero;
}icmpv4_urc_hdr_t;
#pragma pack()

#pragma pack(1)
typedef struct icmpv4_error_report_t
{
    icmpv4_hdr_t stIcmpErrReportHdr;
}icmp_err_hdr_t;
#pragma pack()

typedef struct icmpv4_reply_header_st
{

}icmpv4_reply_hdr_t;
extern net_err_t icmpv4_init(void);
extern net_err_t icmpv4_echo_request_out(nbuf_t * nbuf, ip4addr_t * des, uint16_t id, uint16_t seq);
extern net_err_t icmpv4_report_err_out(uint8_t ubType, uint8_t ubCode, nbuf_t * pstNetPacket);
extern net_err_t icmpv4_process_in(netif_t * pstNetif, ip4addr_t * from, nbuf_t * pubNetBuf);
#endif