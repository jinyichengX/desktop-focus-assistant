#ifndef IPGUI_UTILS_H
#define IPGUI_UTILS_H

#define _INTERNEL_
#define __IPGUI_TEST__
#define __IPGUI_INIT__
#define __IPGUI_DEINIT__
#define __IPGUI_API__
#define __IPGUI_STATIC__  static
#define __IPGUI_INLINE__  inline
#define _______________MARKER_______________

#define __IPGUI_MACRO_START         do {
#define __IPGUI_MACRO_END           } while (0)

#define IPGUI_TERNARY_EXEC(cond, true_expr, false_expr) __IPGUI_MACRO_START \
            if (cond) { \
                (true_expr); \
            } else { \
                (false_expr); \
            } \
        __IPGUI_MACRO_END

#ifndef __PERIOD_CALL__
#define __PERIOD_CALL__
#endif

#ifdef __cplusplus
#define IPGUI_HEADER_BEGIN extern "C" {
#define IPGUI_HEADER_END }
#else
#define IPGUI_HEADER_BEGIN
#define IPGUI_HEADER_END
#endif

/* GNUC */
#if defined (__GNUC__)
#define IPGUI_ST_ALIGN(n)   __attribute__((aligned(n)))
#define likely(x)           __builtin_expect(!!(x), 1)
#define unlikely(x)         __builtin_expect(!!(x), 0)
#define __UNUSED__          __attribute__((__unused__))
#define __USED__            __attribute__((__used__))
#define __PACKED__          __attribute__((packed))
#define __PURE__            __attribute__((__pure__))
#define __CONST__           __attribute__((__const__))
#define __NO_RETURN__       __attribute__((__noreturn__))
#define __NAKED__           __attribute__((naked))
#define __WEAK__            __attribute__((weak))
#elif defined (__clang__)
/* standard C */
#if defined __STDC_VERSION__ && (__STDC_VERSION__ >= 201112L)
#define IPGUI_ST_ALIGN(n)   _Alignas(n)
#elif
#define IPGUI_ST_ALIGN(n)   _Pragma("pack(push, 1)")
#else
#error "Unsupported C/C++ standard"
#endif
#elif defined (__CC_ARM)
#error "Unsupported C/C++ compiler"
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)// ARM Compiler V6
#error "Unsupported C/C++ compiler"
#elif defined(__ICCARM__)  || defined(__ICC430__) // __IAR_SYSTEMS_ICC__
#error "Unsupported C/C++ compiler"
#elif defined(__IAR_SYSTEMS_ICC__)// ICC Compiler for STM8/AVR
#error "Unsupported C/C++ compiler"
#else
#define IPGUI_ST_ALIGN(n)
#error "Unsupported C/C++ compiler"
#endif

/* os platform */
#if defined (__alpha__) || defined (__ia64__) || defined (__x86_64__) \
	|| defined (_WIN64) || defined (__LP64__) || defined (__LLP64__)
    #if !defined IPGUI_BASETYPE_64BIT
        #define IPGUI_BASETYPE_64BIT
    #endif
#endif

enum ipgui_aligend_size
{
#ifdef IPGUI_BASETYPE_64BIT
	/* All allocation sizes and addresses are aligned to 8 bytes. */
	IPGUI_MEM_ALIGN_SIZE_MASK = 7U,
    IPGUI_MEM_ALIGN_SIZE      = 8U,
#else
	/* All allocation sizes and addresses are aligned to 4 bytes. */
	IPGUI_MEM_ALIGN_SIZE_MASK = 3U,
    IPGUI_MEM_ALIGN_SIZE      = 4U,
#endif
};
#if defined(IPGUI_BASETYPE_64BIT)
#define ipgui_mem_unit_type_t unsigned long long
#else
#define ipgui_mem_unit_type_t unsigned int
#endif

#define IPGUI_ROUND(x)                  ((int)((x) + 0.5))

#define IPGUI_MAX(x, y)                 (((x) > (y))? (x) : (y))

#define IPGUI_MIN(x, y)                 (((x) < (y))? (x) : (y))

#define IPGUI_ABS(x)                   (((x) < 0)? (-(x)) : (x))

/// max2
#define IPGUI_MAX2(x, y)                 (((x) > (y))? (x) : (y))

/// min2
#define IPGUI_MIN2(x, y)                 (((x) < (y))? (x) : (y))

/// max3
#define IPGUI_MAX3(x, y, z)                 (((x) > (y))? (((x) > (z))? (x) : (z)) : (((y) > (z))? (y) : (z)))

/// min3
#define IPGUI_MIN3(x, y, z)                 (((x) < (y))? (((x) < (z))? (x) : (z)) : (((y) < (z))? (y) : (z)))

/// the number of entries in the array
#define IPGUI_ARRAY_LEN(x)                  ((sizeof((x)) / sizeof((x)[0])))

/// ispow2: 1, 2, 4, 8, 16, 32, ...
#define IPGUI_IS_POW2(x)                    (!((x) & ((x) - 1)) && (x))

/// align2
#define IPGUI_ALIGN2(x)                     (((x) + 1) >> 1 << 1)

/// align4
#define IPGUI_ALIGN4(x)                     (((x) + 3) >> 2 << 2)

/// align8
#define IPGUI_ALIGN8(x)                     (((x) + 7) >> 3 << 3)

/// align
#define IPGUI_ALIGN(x, b)                   (((size_t)(x) + ((size_t)(b) - 1)) & ~((size_t)(b) - 1))

/// align u32
#define IPGUI_ALIGN_U32(x, b)               (((uint32_t)(x) + ((uint32_t)(b) - 1)) & ~((uint32_t)(b) - 1))

/// align u64
#define IPGUI_ALIGN_U64(x, b)               (((uint64_t)(x) + ((uint64_t)(b) - 1)) & ~((uint64_t)(b) - 1))

/// align by pow2
#define IPGUI_ALIGN_POW2(x)                 (((x) > 1)? (IPGUI_IS_POW2(x)? (x) : ((size_t)1 << (32 - tb_bits_cl0_u32_be((tb_uint32_t)(x))))) : 1)

/*
 * align by cpu bytes
 */
#if defined IPGUI_BASETYPE_64BIT
#define IPGUI_ALIGN_CPU(x)                  IPGUI_ALIGN8(x)
#else
#define IPGUI_ALIGN_CPU(x)                  IPGUI_ALIGN4(x)
#endif

/// offsetof
#define IPGUI_OFFSETOF(t, m)                ((size_t) &((t *)0)->m)

/// container of 
#define ipgui_offsetof(type, mem)           ((size_t) &((type *)0)->mem)
#define ipgui_container_of(ptr, type, mem)  (type *)((char *)ptr -ipgui_offsetof(type, mem))

// /// memsizeof
// #define tb_memsizeof(s, m)              sizeof(((s const*)0)->m)

// /// memtailof
// #define tb_memtailof(s, m)              (tb_offsetof(s, m) + tb_memsizeof(s, m))

// /// memdiffof: lm - rm
// #define tb_memdiffof(s, lm, rm)         (tb_memtailof(s, lm) - tb_memtailof(s, rm))

// /// check the offset and size of member for struct or union
// #define tb_memberof_eq(ls, lm, rs, rm)  ((tb_offsetof(ls, lm) == tb_offsetof(rs, rm)) && (tb_memsizeof(ls, lm) == tb_memsizeof(rs, rm)))

/// swap
#define IPGUI_SWAP(t, l, r)                 __IPGUI_MACRO_START t __p = (r); (r) = (l); (l) = __p; __IPGUI_MACRO_END

#define ipgui_writel(a, v)                  (*(volatile unsigned int *)(a) = (v))
#define ipgui_readl(a)                      (*(volatile unsigned int *)(a))

#endif