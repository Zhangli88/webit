/* 
 * File:   wbt_string.h
 * Author: Fcten
 *
 * Created on 2014年8月18日, 下午3:50
 */

#ifndef __WBT_STRING_H__
#define __WBT_STRING_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

#include "wbt_memory.h"

typedef struct {
    size_t  len;
    u_char *str;
} wbt_str_t;

#define wbt_string(str)     { sizeof(str) - 1, (u_char *) str }
#define wbt_null_string     { 0, NULL }

wbt_str_t wbt_sprintf(wbt_mem_t *buf, const char *fmt, ...);

int wbt_strpos( wbt_str_t *str1, wbt_str_t *str2 );
int wbt_strcmp( wbt_str_t *str1, wbt_str_t *str2, int len );

#ifdef	__cplusplus
}
#endif

#endif /* __WBT_STRING_H__ */