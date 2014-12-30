/* 
 * File:   wbt_config.c
 * Author: Fcten
 *
 * Created on 2014年12月16日, 下午3:03
 */

#include "wbt_config.h"

wbt_module_t wbt_module_conf = {
    wbt_string("config"),
    wbt_conf_init
};

wbt_mem_t wbt_config_file_content;

wbt_rbtree_t wbt_config_rbtree;

int wbt_conf_line = 1;
int wbt_conf_charactor = 0;

wbt_status wbt_conf_init() {
    wbt_rbtree_init(&wbt_config_rbtree);
    
    return wbt_conf_reload();
}

static wbt_status wbt_conf_parse(wbt_mem_t * conf) {
    int status = 0;
    u_char *p, ch;
    
    wbt_conf_line = 1;
    wbt_conf_charactor = 0;
    
    wbt_str_t key, value;
    
    //wbt_log_debug("Parse config file: size %d", conf->len);
    
    int i = 0;
    for( i = 0 ; i < conf->len ; i ++ ) {
        p = (u_char *)conf->ptr;
        ch = *(p+i);

        wbt_conf_charactor ++;

        switch( status ) {
            case 0:
                if( ch == ' ' || ch == '\t' ) {
                    /* continue; */
                } else if( ch == '\n' ) {
                    wbt_conf_line ++;
                    wbt_conf_charactor = 0;
                } else if( ch == '#' ) {
                    status = 1;
                } else if( ( ch >= 'a' && ch <= 'z' ) || ch == '_' ) {
                    key.str = p + i;
                    status = 2;
                } else {
                    return WBT_ERROR;
                }
                break;
            case 1:
                if( ch == '\n' ) {
                    wbt_conf_line ++;
                    wbt_conf_charactor = 0;
                    status = 0;
                }
                break;
            case 2:
                if( ( ch >= 'a' && ch <= 'z' ) || ch == '_' ) {
                    /* continue; */
                } else if( ch == ' ' || ch == '\t' ) {
                    key.len = p + i - key.str;
                    status = 3;
                } else {
                    return WBT_ERROR;
                }
                break;
            case 3:
                if( ch == ' ' || ch == '\t' ) {
                    /* continue; */
                } else {
                    value.str = p + i;
                    status = 4;
                }
                break;
            case 4:
                if( ch == '\n' ) {
                    wbt_conf_line ++;
                    wbt_conf_charactor = 0;
                    value.len = p + i - value.str;
                    status = 0;
                    
                    /* 储存配置信息 */
                    wbt_rbtree_node_t *option =  wbt_rbtree_get(&wbt_config_rbtree, &key);
                    if( option == NULL ) {
                        /* 新的值 */
                        option = wbt_rbtree_insert(&wbt_config_rbtree, &key);
                    } else {
                        /* 已有的值 */
                        wbt_free(&option->value);
                    }
                    wbt_malloc(&option->value, value.len);
                    /* 直接把 (wbt_str_t *) 转换为 (wbt_mem_t *) 会导致越界访问，
                     * 不过目前我认为这不是问题 */
                    wbt_memcpy(&option->value, (wbt_mem_t *)&value, value.len);
                }
                break;
            default:
                /* 不应当出现未知的状态 */
                return WBT_ERROR;
        }
    }
    
    return WBT_OK;
}

wbt_status wbt_conf_reload() {
    /* TODO 清理已保存的配置信息 */

    /* 尝试读取配置文件 */
    wbt_str_t wbt_config_file_name = wbt_string("./webit.conf");
    
    wbt_file_t wbt_config_file;
    wbt_config_file.fd = open(wbt_config_file_name.str, O_RDONLY);
    
    if( wbt_config_file.fd <= 0 ) {
        /* 找不到配置文件 */
        wbt_log_add("Can't find config file: %.*s\n", wbt_config_file_name.len, wbt_config_file_name.str);

        return WBT_ERROR;
    }
    
    struct stat statbuff;  
    if(stat(wbt_config_file_name.str, &statbuff) < 0){
        wbt_config_file.size = 0;  
    }else{  
        wbt_config_file.size = statbuff.st_size;  
    }

    wbt_malloc(&wbt_config_file_content, wbt_config_file.size);
    read(wbt_config_file.fd, wbt_config_file_content.ptr, wbt_config_file_content.len);
    
    /* 关闭配置文件 */
    close(wbt_config_file.fd);
    
    /* 解析配置文件 */
    if( wbt_conf_parse(&wbt_config_file_content) == WBT_OK ) {
        wbt_free(&wbt_config_file_content);
        wbt_rbtree_print(wbt_config_rbtree.root);
        return WBT_OK;
    } else {
        wbt_free(&wbt_config_file_content);
        wbt_log_add("Syntax error on config file: line %d, charactor %d\n", wbt_conf_line, wbt_conf_charactor);
        return WBT_ERROR;
    }
}

const char * wbt_conf_get( const char * name ) {
    wbt_str_t config_name;
    config_name.str = (u_char *)name;
    config_name.len = strlen(name);
    wbt_rbtree_node_t * root = wbt_rbtree_get(&wbt_config_rbtree, &config_name);
    if( root == NULL ) {
        return NULL;
    } else {
        return wbt_stdstr( (wbt_str_t *)&root->value );
    }
}

wbt_mem_t * wbt_conf_get_v( const char * name ) {
    wbt_str_t config_name;
    config_name.str = (u_char *)name;
    config_name.len = strlen(name);
    wbt_rbtree_node_t * root = wbt_rbtree_get(&wbt_config_rbtree, &config_name);
    if( root == NULL ) {
        return NULL;
    } else {
        return &root->value;
    }
}