/* 
 * File:   webit.h
 * Author: Fcten
 *
 * Created on 2014年8月20日, 下午3:21
 */

#ifndef __WEBIT_H__
#define	__WEBIT_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#define WBT_DEBUG

#define WBT_MAX_EVENTS      256
#define WBT_EVENT_LIST_SIZE 1024
#define WBT_CONN_PORT       1039
#define WBT_CONN_BACKLOG    511
#define WBT_CONN_TIMEOUT    15000    /* 单位毫秒 */

typedef enum {
    WBT_OK,
    WBT_ERROR
} wbt_status;

#ifdef	__cplusplus
}
#endif

#endif	/* __WEBIT_H__ */
