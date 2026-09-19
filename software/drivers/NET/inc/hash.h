#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    HASH_TBL_BUCKET_SIZE_13 = 13,
    HASH_TBL_BUCKET_SIZE_43 = 43,
    HASH_TBL_BUCKET_SIZE_73 = 73,
    HASH_TBL_BUCKET_SIZE_151 = 151,
    HASH_TBL_BUCKET_SIZE_283 = 283,
    HASH_TBL_BUCKET_SIZE_571 = 571,
    HASH_TBL_BUCKET_SIZE_1153 = 1153,
    HASH_TBL_BUCKET_SIZE_2269 = 2269,
    HASH_TBL_BUCKET_SIZE_4519 = 4519,
    HASH_TBL_BUCKET_SIZE_9013 = 9013,
    HASH_TBL_BUCKET_SIZE_18043 = 18043,
    HASH_TBL_BUCKET_SIZE_36109 = 36109,
    HASH_TBL_BUCKET_SIZE_72091 = 72091,
    HASH_TBL_BUCKET_SIZE_144409 = 144409,
    HASH_TBL_BUCKET_SIZE_288361 = 288361,
    HASH_TBL_BUCKET_SIZE_576883 = 576883,
    HASH_TBL_BUCKET_SIZE_1153459 = 1153459,
    HASH_TBL_BUCKET_SIZE_2307163 = 2307163,
    HASH_TBL_BUCKET_SIZE_4613893 = 4613893,
    HASH_TBL_BUCKET_SIZE_9227641 = 9227641,
    HASH_TBL_BUCKET_SIZE_18455029 = 18455029,
    HASH_TBL_BUCKET_SIZE_36911011 = 36911011,
    HASH_TBL_BUCKET_SIZE_73819861 = 73819861,
    HASH_TBL_BUCKET_SIZE_147639589 = 147639589,
    HASH_TBL_BUCKET_SIZE_295279081 = 295279081,
    HASH_TBL_BUCKET_SIZE_590559793 = 590559793
}hash_tbl_bucket_size_t;

typedef size_t (*hash_func_t)(unsigned char * key, size_t len, size_t seed);

typedef struct hash_node_st
{
    struct hash_node_st * next;
    unsigned char * key;
    void * value;
} hash_node_t;

typedef struct hash_table_st
{
    hash_func_t hash_func;
    hash_node_t ** buckets;     //hash桶索引
    size_t key_len;             //统计key的长度
    int bucket_count;           //统计hash桶的数量
    int node_count;
}hash_tbl_t;
extern size_t murmur_make(unsigned char * data, size_t size, size_t seed);
extern int hash_init(hash_tbl_t * hash_tbl, hash_func_t, size_t key_len, hash_tbl_bucket_size_t bksz);
extern int hash_insert(hash_tbl_t * hash_tbl, unsigned char * key, void * val);
extern int hash_search(hash_tbl_t * hash_tbl, unsigned char * key, void ** val);
extern int hash_modify(hash_tbl_t * hash_tbl, unsigned char * key, void * val_new);
extern int hash_delete(hash_tbl_t * hash_tbl, unsigned char * key);
#endif