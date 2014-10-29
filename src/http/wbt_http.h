/* 
 * File:   wbt_http.h
 * Author: Fcten
 *
 * Created on 2014年10月24日, 下午3:11
 */

#ifndef __WBT_HTTP_H__
#define	__WBT_HTTP_H__

#ifdef	__cplusplus
extern "C" {
#endif


#include "../webit.h"
#include "../common/wbt_memory.h"
#include "../common/wbt_string.h"

typedef enum { 
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    METHOD_PUT, 
    METHOD_DELETE, 
    METHOD_TRACE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_LENGTH
} wbt_http_method_t;

typedef enum {
    HEADER_CACHE_CONTROL,
    HEADER_CONNECTION,
    HEADER_DATE,
    HEADER_PRAGMA,
    HEADER_TRAILER,
    HEADER_TRANSFER_ENCODING,
    HEADER_UPGRADE,
    HEADER_VIA,
    HEADER_WARNING,
    HEADER_ACCEPT,
    HEADER_ACCEPT_CHARSET,
    HEADER_ACCEPT_ENCODING,
    HEADER_ACCEPT_LANGUAGE,
    HEADER_AUTHORIZATION,
    HEADER_EXPECT,
    HEADER_FROM,
    HEADER_HOST,
    HEADER_IF_MATCH,
    HEADER_IF_MODIFIED_SINCE,
    HEADER_IF_NONE_MATCH,
    HEADER_IF_RANGE,
    HEADER_IF_UNMODIFIED_SINCE,
    HEADER_MAX_FORWARDS,
    HEADER_PROXY_AUTHORIZATION,
    HEADER_RANGE,
    HEADER_REFERER,
    HEADER_TE,
    HEADER_USER_AGENT,
    HEADER_ACCEPT_RANGES,
    HEADER_AGE,
    HEADER_ETAG,
    HEADER_LOCATION,
    HEADER_PROXY_AUTENTICATE,
    HEADER_RETRY_AFTER,
    HEADER_SERVER,
    HEADER_VARY,
    HEADER_WWW_AUTHENTICATE,
    HEADER_ALLOW,
    HEADER_CONTENT_ENCODING,
    HEADER_CONTENT_LANGUAGE,
    HEADER_CONTENT_LENGTH,
    HEADER_CONTENT_LOCATION,
    HEADER_CONTENT_MD5,
    HEADER_CONTENT_RANGE,
    HEADER_CONTENT_TYPE,
    HEADER_EXPIRES,
    HEADER_LAST_MODIFIED,
    HEADER_COOKIE,
    HEADER_SET_COOKIE,
    HEADER_LENGTH
} wbt_http_line_t;

typedef enum {
    STATUS_100,  // Continue
    STATUS_101,  // Switching Protocols
    STATUS_102,  // Processing
    STATUS_200,  // OK
    STATUS_201,  // Created
    STATUS_202,  // Accepted
    STATUS_203,  // Non_Authoritative Information
    STATUS_204,  // No Content
    STATUS_205,  // Reset Content
    STATUS_206,  // Partial Content
    STATUS_207,  // Multi-Status
    STATUS_300,  // Multiple Choices
    STATUS_301,  // Moved Permanently
    STATUS_302,  // Found
    STATUS_302_1,// Moved Temporarily
    STATUS_303,  // See Other
    STATUS_304,  // Not Modified
    STATUS_305,  // Use Proxy
    STATUS_307,  // Temporary Redirect
    STATUS_400,  // Bad Request
    STATUS_401,  // Unauthorized
    STATUS_402,  // Payment Required
    STATUS_403,  // Forbidden
    STATUS_404,  // Not Found
    STATUS_405,  // Method Not Allowed
    STATUS_406,  // Not Acceptable
    STATUS_407,  // Proxy Authentication Required
    STATUS_408,  // Request Time_out
    STATUS_409,  // Conflict
    STATUS_410,  // Gone
    STATUS_411,  // Length Required
    STATUS_412,  // Precondition Failed
    STATUS_413,  // Request Entity Too Large
    STATUS_414,  // Request_URI Too Large
    STATUS_415,  // Unsupported Media Type
    STATUS_416,  // Requested range not satisfiable
    STATUS_417,  // Expectation Failed
    STATUS_422,  // Unprocessable Entity
    STATUS_423,  // Locked
    STATUS_424,  // Failed Dependency
    STATUS_426,  // Upgrade Required
    STATUS_500,  // Internal Server Error
    STATUS_501,  // Not Implemented
    STATUS_502,  // Bad Gateway
    STATUS_503,  // Service Unavailable
    STATUS_504,  // Gateway Time_out
    STATUS_505,  // HTTP Version not supported
    STATUS_506,  // Variant Also Negotiates
    STATUS_507,  // Insufficient Storage
    STATUS_510,  // Not Extended
    STATUS_LENGTH
} wbt_http_status_t;

typedef struct wbt_http_header_s {
    wbt_http_line_t key;
    wbt_str_t name;
    wbt_str_t value;
    struct wbt_http_header_s * next;
} wbt_http_header_t;
    
typedef struct wbt_http_s {
    wbt_mem_t buff;
    wbt_str_t method;
    wbt_str_t uri;
    wbt_str_t version;
    wbt_http_header_t * headers;
    wbt_str_t body;
} wbt_http_t;

extern wbt_str_t REQUEST_METHOD[];
extern wbt_str_t HTTP_HEADERS[];
extern wbt_str_t STATUS_CODE[];

wbt_status wbt_http_check_header_end( wbt_http_t* );
wbt_status wbt_http_parse_request_header( wbt_http_t* );
wbt_status wbt_http_check_body_end( wbt_http_t* );

wbt_status wbt_http_new( wbt_http_t* );
wbt_status wbt_http_parse( wbt_http_t* );
wbt_status wbt_http_destroy( wbt_http_t* );

wbt_status wbt_http_set_secure( wbt_http_t*, int );
wbt_status wbt_http_set_method( wbt_http_t*, int );
wbt_status wbt_http_set_uri( wbt_http_t*, const char * );
wbt_status wbt_http_set_version( wbt_http_t*, const char * );
wbt_status wbt_http_set_header( wbt_http_t*, int, const char * );
wbt_status wbt_http_set_address( wbt_http_t*, const char *, const unsigned short int );
wbt_status wbt_http_set_body( wbt_http_t*, const char * );

wbt_status wbt_http_get_body( wbt_http_t* );
wbt_status wbt_http_get_header( wbt_http_t*, int );


#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_HTTP_H__ */
