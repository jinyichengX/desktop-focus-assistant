
/*
 * MIT License
 *
 * Copyright (c) 2025 JinYiCheng
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

/* user include here begin */
#include <stdio.h>
/* user include here end */

typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned int uint32_t;
typedef int int32_t;

#if defined(_WIN64)
    typedef uint64_t size_t;
    typedef int64_t ssize_t;
#else
    typedef uint32_t size_t;
    typedef int32_t ssize_t;
#endif

typedef char * va_list;
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define va_start(ap, fmt) ap = (va_list)&fmt + _INTSIZEOF(fmt)
#define va_arg(ap, type) (*(type *)(ap += _INTSIZEOF(type), ap - _INTSIZEOF(type)))
#define va_end(v) v = (va_list)0

static int strlen(const char * str)
{
    int len;
    while(* str ++ != '\0') len ++;
    return len;
}

static char * strcpy(char * dst, const char * src)
{
    char * tmp = dst;
    while (*src != '\0') * dst ++ = * src ++;
    return tmp;
}

static void putck(char c)
{
//    putchar(c);
}

static void putsk(char * s)
{
    while (*s)
        putck(*s ++);
}

/* upper used for x/X */
static int itoak(ssize_t value, int unsign, char * str, int radix, int upper)
{
    /* rodata section */
    static const char hex_tbl[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
        'A', 'B', 'C', 'D', 'E', 'F'  
    };
    char * p = str;
    int negative = 0, len = 0;
    size_t val = value;

    if (!value) {
        * p ++ = '0';
        * p ++ = '\0';
        return 1;
    }

    if ((radix > 16)  || (radix < 0))
        return -1;
    
    if (value < 0) {
        if (!unsign) {
            negative = 1;
            val = value = -value;
        }else {
            val = (size_t)value;
        }
    }

    while (val > 0) {
        char idx = val % radix;
        *(p ++) = hex_tbl[(idx < 10) ? idx : (upper ? (idx + 6) : idx)];
        val /= radix;
    }
    if (negative)
        *(p ++) = '-';

    len = p - str;
    for (int i = 0; i < len / 2; i ++) {
        char j = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = j;
    }
    * p = '\0';

    return len;
}

static void print_number(ssize_t val, int unsign, int radix, int upper)
{
    static char bf[33];
    if (itoak(val, unsign, bf, radix, upper))
        putsk(bf);
}

static int vsprintk(char * str, const char * fmt, va_list arg)
{
    char * tmp = str;
    char * p;
    for (; *fmt != '\0'; fmt ++) 
    {
        if (*fmt != '%')
        {
            * str ++ = * fmt;
            continue;
        }
        fmt ++;

        /* format string */
        switch (*fmt)
        {
            case 'd':
            case 'i': 
                str += itoak
                (va_arg(arg, size_t), 0, str, 10, 0);
                break;
            case 'u':
                str += itoak
                (va_arg(arg, size_t), 1, str, 10, 0);
                break;
            case 'o':
                str += itoak
                (va_arg(arg, ssize_t), 0, str, 8, 0);
                break;
            case 'p':
                str += itoak
                (va_arg(arg, size_t), 1, str, 16, 0);
                break;
            case 'x':      
                str += itoak
                (va_arg(arg, ssize_t), 0, str, 16, 0);
                break;
            case 'X':
                str += itoak
                (va_arg(arg, ssize_t), 0, str, 16, 1);
                break;
            case 's':
                p = va_arg(arg, char *);
                strcpy(str, p);
                str += strlen(p);
                break;
            case 'c':
                * str ++ = \
                va_arg(arg, char);
                break;
            case '%':
                * str ++ = '%';
                break;
            default:
                break;
        }
    }
    return str - tmp;
}

/* return length of str */
int sprintk(char * str, const char * fmt, ...)
{
    va_list arg;
    int size;
    va_start(arg, fmt);
    size = vsprintk(str, fmt, arg);
    va_end(arg);
    return size;
}

/* return length of str,else return -1 */
static int vprintk(char * fmt, va_list arg)
{
    for (; * fmt != '\0'; fmt ++) 
    {
        if (* fmt != '%')
        {
            putck(* fmt);
            continue;
        }
        fmt ++;

        /* format string */
        switch (* fmt)
        {
            case 'd':
            case 'i': 
                print_number
                (va_arg(arg, ssize_t), 0, 10, 0);
                break;
            case 'u':
                print_number
                (va_arg(arg, size_t), 1, 10, 0);
                break;
            case 'o':
                print_number
                (va_arg(arg, ssize_t), 0, 8, 0);
                break;
            case 'p':
                print_number
                (va_arg(arg, size_t), 1, 16, 0);
                break;
            case 'x':                
                print_number
                (va_arg(arg, ssize_t), 0, 16, 0);
                break;
            case 'X':
                print_number
                (va_arg(arg, ssize_t), 0, 16, 1);
                break;
            case 's':
                putsk
                (va_arg(arg, char *));
                break;
            case 'c':
                putck
                (va_arg(arg, char));
                break;
            case '%':
                putck('%');
                break;
            default:
                putck(* fmt);
                break;
        }
    }
    return 0;
}

void printk(char * fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintk(fmt, ap);
    va_end(ap);
}