
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

#include "hash.h"
#include "nmem.h"

static const unsigned int hash_table_sizes[] = {
    13,
    43,
    73,
    151,
    283,
    571,
    1153,
    2269,
    4519,
    9013,
    18043,
    36109,
    72091,
    144409,
    288361,
    576883,
    1153459,
    2307163,
    4613893,
    9227641,
    18455029,
    36911011,
    73819861,
    147639589,
    295279081,
    590559793
};


/* crc16 for hash */
uint16_t const g_crc16_table[] =
{
    0x0000, 0x0580, 0x0f80, 0x0a00, 0x1b80, 0x1e00, 0x1400, 0x1180
,	0x3380, 0x3600, 0x3c00, 0x3980, 0x2800, 0x2d80, 0x2780, 0x2200
,	0x6380, 0x6600, 0x6c00, 0x6980, 0x7800, 0x7d80, 0x7780, 0x7200
,	0x5000, 0x5580, 0x5f80, 0x5a00, 0x4b80, 0x4e00, 0x4400, 0x4180
,	0xc380, 0xc600, 0xcc00, 0xc980, 0xd800, 0xdd80, 0xd780, 0xd200
,	0xf000, 0xf580, 0xff80, 0xfa00, 0xeb80, 0xee00, 0xe400, 0xe180
,	0xa000, 0xa580, 0xaf80, 0xaa00, 0xbb80, 0xbe00, 0xb400, 0xb180
,	0x9380, 0x9600, 0x9c00, 0x9980, 0x8800, 0x8d80, 0x8780, 0x8200
,	0x8381, 0x8601, 0x8c01, 0x8981, 0x9801, 0x9d81, 0x9781, 0x9201
,	0xb001, 0xb581, 0xbf81, 0xba01, 0xab81, 0xae01, 0xa401, 0xa181
,	0xe001, 0xe581, 0xef81, 0xea01, 0xfb81, 0xfe01, 0xf401, 0xf181
,	0xd381, 0xd601, 0xdc01, 0xd981, 0xc801, 0xcd81, 0xc781, 0xc201
,	0x4001, 0x4581, 0x4f81, 0x4a01, 0x5b81, 0x5e01, 0x5401, 0x5181
,	0x7381, 0x7601, 0x7c01, 0x7981, 0x6801, 0x6d81, 0x6781, 0x6201
,	0x2381, 0x2601, 0x2c01, 0x2981, 0x3801, 0x3d81, 0x3781, 0x3201
,	0x1001, 0x1581, 0x1f81, 0x1a01, 0x0b81, 0x0e01, 0x0401, 0x0181
,	0x0383, 0x0603, 0x0c03, 0x0983, 0x1803, 0x1d83, 0x1783, 0x1203
,	0x3003, 0x3583, 0x3f83, 0x3a03, 0x2b83, 0x2e03, 0x2403, 0x2183
,	0x6003, 0x6583, 0x6f83, 0x6a03, 0x7b83, 0x7e03, 0x7403, 0x7183
,	0x5383, 0x5603, 0x5c03, 0x5983, 0x4803, 0x4d83, 0x4783, 0x4203
,	0xc003, 0xc583, 0xcf83, 0xca03, 0xdb83, 0xde03, 0xd403, 0xd183
,	0xf383, 0xf603, 0xfc03, 0xf983, 0xe803, 0xed83, 0xe783, 0xe203
,	0xa383, 0xa603, 0xac03, 0xa983, 0xb803, 0xbd83, 0xb783, 0xb203
,	0x9003, 0x9583, 0x9f83, 0x9a03, 0x8b83, 0x8e03, 0x8403, 0x8183
,	0x8002, 0x8582, 0x8f82, 0x8a02, 0x9b82, 0x9e02, 0x9402, 0x9182
,	0xb382, 0xb602, 0xbc02, 0xb982, 0xa802, 0xad82, 0xa782, 0xa202
,	0xe382, 0xe602, 0xec02, 0xe982, 0xf802, 0xfd82, 0xf782, 0xf202
,	0xd002, 0xd582, 0xdf82, 0xda02, 0xcb82, 0xce02, 0xc402, 0xc182
,	0x4382, 0x4602, 0x4c02, 0x4982, 0x5802, 0x5d82, 0x5782, 0x5202
,	0x7002, 0x7582, 0x7f82, 0x7a02, 0x6b82, 0x6e02, 0x6402, 0x6182
,	0x2002, 0x2582, 0x2f82, 0x2a02, 0x3b82, 0x3e02, 0x3402, 0x3182
,	0x1382, 0x1602, 0x1c02, 0x1982, 0x0802, 0x0d82, 0x0782, 0x0202
};

uint16_t const g_crc16_ccitt_table[] =
{
    0x0000, 0x2110, 0x4220, 0x6330, 0x8440, 0xa550, 0xc660, 0xe770
,	0x0881, 0x2991, 0x4aa1, 0x6bb1, 0x8cc1, 0xadd1, 0xcee1, 0xeff1
,	0x3112, 0x1002, 0x7332, 0x5222, 0xb552, 0x9442, 0xf772, 0xd662
,	0x3993, 0x1883, 0x7bb3, 0x5aa3, 0xbdd3, 0x9cc3, 0xfff3, 0xdee3
,	0x6224, 0x4334, 0x2004, 0x0114, 0xe664, 0xc774, 0xa444, 0x8554
,	0x6aa5, 0x4bb5, 0x2885, 0x0995, 0xeee5, 0xcff5, 0xacc5, 0x8dd5
,	0x5336, 0x7226, 0x1116, 0x3006, 0xd776, 0xf666, 0x9556, 0xb446
,	0x5bb7, 0x7aa7, 0x1997, 0x3887, 0xdff7, 0xfee7, 0x9dd7, 0xbcc7
,	0xc448, 0xe558, 0x8668, 0xa778, 0x4008, 0x6118, 0x0228, 0x2338
,	0xccc9, 0xedd9, 0x8ee9, 0xaff9, 0x4889, 0x6999, 0x0aa9, 0x2bb9
,	0xf55a, 0xd44a, 0xb77a, 0x966a, 0x711a, 0x500a, 0x333a, 0x122a
,	0xfddb, 0xdccb, 0xbffb, 0x9eeb, 0x799b, 0x588b, 0x3bbb, 0x1aab
,	0xa66c, 0x877c, 0xe44c, 0xc55c, 0x222c, 0x033c, 0x600c, 0x411c
,	0xaeed, 0x8ffd, 0xeccd, 0xcddd, 0x2aad, 0x0bbd, 0x688d, 0x499d
,	0x977e, 0xb66e, 0xd55e, 0xf44e, 0x133e, 0x322e, 0x511e, 0x700e
,	0x9fff, 0xbeef, 0xdddf, 0xfccf, 0x1bbf, 0x3aaf, 0x599f, 0x788f
,	0x8891, 0xa981, 0xcab1, 0xeba1, 0x0cd1, 0x2dc1, 0x4ef1, 0x6fe1
,	0x8010, 0xa100, 0xc230, 0xe320, 0x0450, 0x2540, 0x4670, 0x6760
,	0xb983, 0x9893, 0xfba3, 0xdab3, 0x3dc3, 0x1cd3, 0x7fe3, 0x5ef3
,	0xb102, 0x9012, 0xf322, 0xd232, 0x3542, 0x1452, 0x7762, 0x5672
,	0xeab5, 0xcba5, 0xa895, 0x8985, 0x6ef5, 0x4fe5, 0x2cd5, 0x0dc5
,	0xe234, 0xc324, 0xa014, 0x8104, 0x6674, 0x4764, 0x2454, 0x0544
,	0xdba7, 0xfab7, 0x9987, 0xb897, 0x5fe7, 0x7ef7, 0x1dc7, 0x3cd7
,	0xd326, 0xf236, 0x9106, 0xb016, 0x5766, 0x7676, 0x1546, 0x3456
,	0x4cd9, 0x6dc9, 0x0ef9, 0x2fe9, 0xc899, 0xe989, 0x8ab9, 0xaba9
,	0x4458, 0x6548, 0x0678, 0x2768, 0xc018, 0xe108, 0x8238, 0xa328
,	0x7dcb, 0x5cdb, 0x3feb, 0x1efb, 0xf98b, 0xd89b, 0xbbab, 0x9abb
,	0x754a, 0x545a, 0x376a, 0x167a, 0xf10a, 0xd01a, 0xb32a, 0x923a
,	0x2efd, 0x0fed, 0x6cdd, 0x4dcd, 0xaabd, 0x8bad, 0xe89d, 0xc98d
,	0x267c, 0x076c, 0x645c, 0x454c, 0xa23c, 0x832c, 0xe01c, 0xc10c
,	0x1fef, 0x3eff, 0x5dcf, 0x7cdf, 0x9baf, 0xbabf, 0xd98f, 0xf89f
,	0x176e, 0x367e, 0x554e, 0x745e, 0x932e, 0xb23e, 0xd10e, 0xf01e
};

uint16_t crc16_make(uint8_t const * data, size_t size, uint16_t seed)
{
    uint32_t crc = seed;//seed取0xffff
    uint8_t const * ie = data + size;
    uint16_t const * pt = (uint16_t const *)g_crc16_table;
    while (data < ie) crc = pt[((uint8_t)crc) ^ *data++] ^ (crc >> 8);
    return (uint16_t)crc;
}

uint16_t crc16_ccitt_make(uint8_t const * data, size_t size, uint16_t seed)
{
    uint16_t crc = seed;//seed取0xffff
    uint8_t const * ie = data + size;
    uint16_t const * pt = (uint16_t const *)g_crc16_ccitt_table;
    while (data < ie) crc = pt[((uint8_t)crc) ^ *data++] ^ (crc >> 8);
    return crc;
}

/* murmur for hash */
size_t murmur_make(unsigned char * data, size_t size, size_t seed)
{
    size_t value = seed;
    while (size--)
    {
        value ^= (*data++);
        value *= 0x5bd1e995;
        value ^= value >> 15;
    }
    return value;
}

/* ap for hash */
size_t ap_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t i = 0;
    size_t v = 0xAAAAAAAA;
    if (seed)
        v ^= seed;
    uint8_t const * p = data;
    for (i = 0; i < size; i ++, p ++)
    {
        v ^= (!(i & 1)) ? ((v << 7) ^ ((*p) * (v >> 3))) : (~(((v << 11) + (*p)) ^ (v >> 5)));
    }
    return v;
}

/* bkdr for hash */
size_t bkdr_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t value = 0;
    if (seed)
        value = value * 131313 + seed;

    while (size--)
        value = (value * 131313) + (*data++);
    return value;
}

#if 0
/* blizzard for hash(暴雪hash) */
size_t blizzard_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t value = seed;

    // generate it
    while (size--)
        value = (*data++) + (value << 6) + (value << 16) - value;
    return value;

    // make table
    static size_t s_make = 0;
    static size_t s_table[1280];//??游戏用的hash就离谱
    if (!s_make)
    {
        size_t i = 0;
        size_t index1 = 0;
        size_t index2 = 0;
        size_t seed0 = 0x00100001;
        for (index1 = 0; index1 < 0x100; index1++)
        {
            for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100)
            {
                seed0 = (seed0 * 125 + 3) % 0x2aaaab; size_t temp1 = (seed0 & 0xffff) << 0x10;
                seed0 = (seed0 * 125 + 3) % 0x2aaaab; size_t temp2 = (seed0 & 0xffff);
                s_table[index2] = (temp1 | temp2);
            }
        }

        // ok
        s_make = 1;
    }

    // init value
    size_t seed1 = 0x7fed7fed;
    size_t seed2 = 0Xeeeeeeee;
    if (seed)
    {
        seed1 = s_table[(1 << 8) + seed] ^ (seed1 + seed2);
        seed2 = seed + seed1 + seed2 + (seed2 << 5) + 3;
    }

    // done
    size_t byte = 0;
    while (size--)
    {
        // get one byte
        byte = *data++;

        // 0 << 8: hash type: 0
        // 1 << 8: hash type: 1
        // 2 << 8: hash type: 2
        seed1 = s_table[(1 << 8) + byte] ^ (seed1 + seed2);
        seed2 = byte + seed1 + seed2 + (seed2 << 5) + 3;
    }

    return seed1;
}
#endif

/* djb2 for hash */
size_t djb2_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t value = 5381;
    if (seed) value = value * 33 + seed;

    while (size--) value = (value * 33) + (*data++);
    return value;
}

/* fnv32 for hash */
#define TB_FNV32_PRIME          (16777619)
#define TB_FNV32_OFFSET_BASIS   (2166136261)
uint32_t fnv32_make(uint8_t const * data, size_t size, uint32_t seed)
{
    uint32_t value = TB_FNV32_OFFSET_BASIS;
    if (seed) value = (value * TB_FNV32_PRIME) ^ seed;

    while (size)
    {
        value *= TB_FNV32_PRIME;
        value ^= (uint32_t)*data++;
        size--;
    }
    return value;
}

uint32_t fnv32_1a_make(uint8_t const * data, size_t size, uint32_t seed)
{
    uint32_t value = TB_FNV32_OFFSET_BASIS;
    if (seed) value = (value * TB_FNV32_PRIME) ^ seed;

    while (size)
    {
        value ^= (uint32_t)*data++;
        value *= TB_FNV32_PRIME;
        size--;
    }
    return value;
}

/* rs for hash */
size_t rs_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t value = seed;

    size_t b = 378551;
    size_t a = 63689;
    while (size--)
    {
        value = value * a + (*data++);
        a = a * b;
    }
    return value;
}

/* sdbm for hash */
size_t sdbm_make(uint8_t const * data, size_t size, size_t seed)
{
    size_t value = seed;

    while (size--) value = (*data++) + (value << 6) + (value << 16) - value;
    return value;
}

/* md5 (for tcp isn) */
#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef unsigned long int UINT4;

#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

typedef struct {
    UINT4 state[4];                                   /* state (ABCD) */
    UINT4 count[2];                                   /* 位数量, 模 2^64 (低位在前) */
    unsigned char buffer[64];                         /* 输入缓冲器 */
} MD5_CTX;

void MD5Init PROTO_LIST ((MD5_CTX *));
void MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));

extern mem_mng_t g_mem_mng;
#define USE_LIBC_MEM 0

int hash_init(hash_tbl_t * hash_tbl, hash_func_t hash_func, size_t key_len, hash_tbl_bucket_size_t bksz)
{
    int i;
    hash_tbl->buckets = NULL;
#if USE_LIBC_MEM
    hash_tbl->buckets = (hash_node_t **)malloc(bksz * sizeof(hash_node_t *));
#else
    hash_tbl->buckets = (hash_node_t **)mem_alloc(&g_mem_mng, bksz * sizeof(hash_node_t *));
#endif
    if( hash_tbl->buckets == NULL )
        return 1; 

    hash_tbl->key_len = key_len;
    hash_tbl->bucket_count = bksz;
    hash_tbl->hash_func = hash_func;
    
    for(i = 0; i < bksz; i++)
        hash_tbl->buckets[i] = NULL;
    
    return 0;
}

/* 存放tcp/udp控制块时，"四元组"----"tcp/udp控制块"构成键值对 */
/* 键必须是唯一的，对应四元组 */
int hash_insert(hash_tbl_t * hash_tbl, unsigned char * key, void * val)
{
    int idx;

    hash_node_t * node = NULL;
    hash_node_t * bucket;
    hash_node_t ** iter;
#if USE_LIBC_MEM
    node = (hash_node_t *)malloc(sizeof(hash_node_t));
#else
    node = (hash_node_t *)mem_alloc(&g_mem_mng, sizeof(hash_node_t));
#endif
    if( node == NULL )
        return -1;

    node->key = key;
    node->value = val;
    node->next = NULL;

    idx = hash_tbl->hash_func(key, hash_tbl->key_len, 0) % hash_tbl->bucket_count;
    bucket = hash_tbl->buckets[idx];
    iter = &bucket;
    while(NULL != (*iter))
    {
        if(0 == memcmp(node->key, (*iter)->key, hash_tbl->key_len))
        {
#if USE_LIBC_MEM
            free(node);
#else
            mem_free(&g_mem_mng, node);
#endif
            return 1;
        }
        iter = &((*iter)->next);
    }

    node->next = bucket;
    hash_tbl->buckets[idx] = node;

    return 0;
}

int hash_delete(hash_tbl_t * hash_tbl, unsigned char * key)
{
    int idx;
    hash_node_t ** iter;
    void * p;

    idx = hash_tbl->hash_func(key, hash_tbl->key_len, 0) % hash_tbl->bucket_count;
    iter = &(hash_tbl->buckets[idx]);

    while (NULL != (*iter))
    {
        if (0 == memcmp((*iter)->key, key, hash_tbl->key_len))
        {
            p = (void *)(* iter);
            *iter = (*iter)->next;
#if USE_LIBC_MEM
            free(p);
#else
            mem_free(&g_mem_mng, p);
#endif
            return 0;
        }
        iter = &((*iter)->next);
    }

    return 1;
}

int hash_search(hash_tbl_t * hash_tbl, unsigned char * key, void ** val)
{
    int idx;
    hash_node_t ** iter;

    idx = hash_tbl->hash_func(key, hash_tbl->key_len, 0) % hash_tbl->bucket_count;
    iter = &(hash_tbl->buckets[idx]);

    while (NULL != (*iter))
    {
        if (0 == memcmp((*iter)->key, key, hash_tbl->key_len))
        {
            * val = (*iter)->value;
            return 0;
        }
        iter = &((*iter)->next);
    }

    return 1;
}

int hash_modify(hash_tbl_t * hash_tbl, unsigned char * key, void * val_new)
{
    int idx;
    hash_node_t ** iter;

    idx = hash_tbl->hash_func(key, hash_tbl->key_len, 0) % hash_tbl->bucket_count;
    iter = &(hash_tbl->buckets[idx]);

    while (NULL != (*iter))
    {
        if (0 == memcmp( (*iter)->key, key, hash_tbl->key_len))
        {
            (*iter)->value = val_new;
            return 0;
        }
        iter = &((*iter)->next);
    }

    return 1;
}
/*    net_sock_t sock1 = {
        .src.ip4addr.ipa = {192,168,1,5},
        .src.port = 8080,
        .dst.ip4addr.ipa = {192,168,1,3},
        .src.port = 8080,
    };
    net_sock_t sock2 = {
        .src.ip4addr.ipa = {192,168,1,5},
        .src.port = 8080,
        .dst.ip4addr.ipa = {192,168,1,3},
        .src.port = 8899,
    };
    net_sock_t sock3 = {
        .src.ip4addr.ipa = {192,168,1,5},
        .src.port = 8080,
        .dst.ip4addr.ipa = {192,168,1,3},
        .src.port = 8899,
    };

    if(0 != hash_init(&hash1, murmur_make, QUADRUPLE_SIZE, HASH_TBL_BUCKET_SIZE_43))
    {   
        printf("hash table create failed\n");
    }
    hash_insert(&hash1, &sock1.src, &sock1);
    hash_insert(&hash1, &sock2.src, &sock2);
    hash_delete(&hash1, &sock2.src);
    hash_insert(&hash1, &sock3.src, &sock3);
*/