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

#include "el_icmpv4.h"

/* icmpv4 init */
net_err_t icmpv4_init(void)
{
    if( ICMPV4_HEADER_LEN != sizeof(icmpv4_hdr_t) )
        return NET_ERR_NOK;

    return NET_ERR_OK;
}

/* icmpv4 echo request out raw */
net_err_t icmpv4_echo_request_out(nbuf_t * nbuf,
                                  ip4addr_t * des,
                                  uint16_t id,
                                  uint16_t seq)
{
    icmp_echo_hdr_t * hdr;

    if (NET_ERR_OK != nbuf_header(nbuf, ICMPV4_HEADER_LEN))
        return NET_ERR_NOK;

    nbuf_extract_prefix(nbuf, ICMPV4_HEADER_LEN);
    hdr = nbuf_data(nbuf);

    /* fill icmpv4 header */
    hdr->type = ICMPV4_TYPE_ECHO_REQUEST;
    hdr->code = ICMPV4_CODE_ECHO_REQUEST;
    hdr->checksum = 0;
    hdr->id = _htons(id);
    hdr->seq = _htons(seq);
    
    hdr->checksum = nbuf_checksum16(nbuf, 0);
    return ipv4_out(IPV4_HEAD_PROTOCAL_ICMP,
    IPV4_HEAD_TTL_DEFAULT, des, NULL, nbuf);
}

/* icmpv4 echo reply out raw */
net_err_t icmpv4_echo_reply_out(netif_t * netif, nbuf_t * nbuf, ip4addr_t * des)
{
    icmp_echo_hdr_t * hdr;
    ip4addr_t * src;
    uint16_t chksum = 0;

    src = &netif->ipaddr;
    nbuf_extract_prefix(nbuf, ICMPV4_HEADER_LEN);
    hdr = nbuf_data(nbuf);

    /* replace type and code */
    hdr->type = ICMPV4_TYPE_ECHO_REPLY;
    hdr->code = ICMPV4_CODE_ECHO_REPLY;

    /* clear check sum 
     * and then recalculate
     */
    hdr->checksum = 0;
    chksum = nbuf_checksum16(nbuf, 0);
    hdr->checksum = chksum;

    return ipv4_out(IPV4_HEAD_PROTOCAL_ICMP,
    IPV4_HEAD_TTL_DEFAULT, des, src, nbuf);
}

/* destination unreachable out raw */
net_err_t icmpv4_urc_out(nbuf_t * nbuf, uint8_t code)
{
    icmpv4_urc_hdr_t * hdr;
    ipv4_hdr_t * ipv4_hdr;
    ip4addr_t des;
    ip4addr_t src;

    /* extract des and src ip first */
    nbuf_extract_prefix(nbuf, IPV4_HEADER_LEN);
    ipv4_hdr = (ipv4_hdr_t *)nbuf_data(nbuf);

    des.ipv = ipv4_hdr->src.ipv;
    src.ipv = ipv4_hdr->des.ipv;

    if (NET_ERR_OK != nbuf_header(nbuf, ICMPV4_HEADER_LEN))
        return NET_ERR_NOK;

    nbuf_extract_prefix(nbuf, ICMPV4_HEADER_LEN);
    hdr = nbuf_data(nbuf);

    /* fill icmpv4 header */
    hdr->type = ICMPV4_TYPE_DEST_UNREACH;
    hdr->code = code;
    hdr->checksum = 0;
    hdr->zero = 0;
    
    hdr->checksum = nbuf_checksum16(nbuf, 0);
    return ipv4_out(IPV4_HEAD_PROTOCAL_ICMP,
    IPV4_HEAD_TTL_DEFAULT, &des, &src, nbuf);
}

/* icmpv4 input */
net_err_t icmpv4_process_in(netif_t * netif, ip4addr_t * from, nbuf_t * nbuf)
{
    uint8_t type, code;

    if (NBUF_TTSZ(nbuf) < ICMPV4_HEADER_LEN)
        goto _error;

    /* extract 8 bytes's header  */
    nbuf_extract_prefix(nbuf, ICMPV4_HEADER_LEN);
    type = *(uint8_t *)nbuf_data(nbuf);
    code = *(uint8_t *)((uint8_t *)nbuf_data(nbuf) + 1);

    /* case of icmp packet type in header */
    switch (type)
    {
        case ICMPV4_TYPE_ECHO_REQUEST:
            if (ICMPV4_CODE_ECHO_REQUEST != code)                           /* error: code error, must be 0 */
                break;
            if (nbuf_checksum16(nbuf, 0))                                   /* error: chksum error */
                break;
            return icmpv4_echo_reply_out(netif, nbuf, from);
        case ICMPV4_TYPE_ECHO_REPLY:
            if (ICMPV4_CODE_ECHO_REPLY != code)                             /* error: code error, must be 0 */
                break;
            if (nbuf_checksum16(nbuf, 0))                                   /* error: chksum error */
                break;
            /* handle icmpv4 echo reply(ping reply) packet */
            //?????   I don't know how to write the code  20250703

        case ICMPV4_TYPE_DEST_UNREACH:
        default: nbuf_free(nbuf); return NET_ERR_OK;
        break;
    }
_error:
    nbuf_free(nbuf);
    return NET_ERR_NOK;
}
